#ifndef K8S_COMMANDS_CREATE_FROM_FILE_H
#define K8S_COMMANDS_CREATE_FROM_FILE_H

#include "modules/kubernetes/module.h"
#include <filesystem>
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace k8s_commands {

inline void bind_matching_volume(KubernetesModule* mod, PersistentVolume& volume) {
    if (volume.status == "Bound" || volume.capacity.empty()) return;

    for (auto& pvc : mod->get_pvcs_mutable()) {
        if (pvc.status == "Bound") continue;
        if (pvc.storage_request != volume.capacity) continue;
        if (!pvc.storage_class.empty() && !volume.storage_class.empty() &&
            pvc.storage_class != volume.storage_class) {
            continue;
        }
        pvc.status = "Bound";
        pvc.volume_name = volume.name;
        volume.status = "Bound";
        volume.claim = pvc.name;
        return;
    }
}

inline auto cmd_create_from_file(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // Prefix "kubectl create -f" is stripped. args begin at the filename.
        if (args.empty() || args[0].empty()) {
            return "Error: filename required\n"
                   "Usage: kubectl create -f <file>\n";
        }

        const std::string path = args[0];
        if (!std::filesystem::exists(path)) {
            return "Error: the path \"" + path + "\" does not exist\n";
        }

        YAML::Node doc;
        try {
            doc = YAML::LoadFile(path);
        } catch (const std::exception& ex) {
            return "Error: failed to parse \"" + path + "\": " + ex.what() + "\n";
        }

        if (!doc || !doc.IsMap()) {
            return "Error: \"" + path + "\" is not a Kubernetes manifest\n";
        }

        const std::string kind = doc["kind"] ? doc["kind"].as<std::string>() : "";
        std::string name;
        if (doc["metadata"] && doc["metadata"]["name"]) {
            name = doc["metadata"]["name"].as<std::string>();
        }
        if (name.empty()) {
            return "Error: metadata.name is required\n";
        }

        if (kind == "NetworkPolicy") {
            for (const auto& existing : mod->get_network_policies()) {
                if (existing.name == name) {
                    return "Error from server (AlreadyExists): networkpolicies.networking.k8s.io \"" +
                           name + "\" already exists\n";
                }
            }
            NetworkPolicy policy;
            policy.name = name;
            policy.age = "0s";
            mod->add_network_policy(policy);
            return "networkpolicy.networking.k8s.io/" + name + " created\n";
        }

        if (kind == "PersistentVolume") {
            for (const auto& existing : mod->get_pvs()) {
                if (existing.name == name) {
                    return "Error from server (AlreadyExists): persistentvolumes \"" + name + "\" already exists\n";
                }
            }

            PersistentVolume volume;
            volume.name = name;
            volume.status = "Available";
            volume.age = "0s";
            if (doc["spec"]) {
                const auto spec = doc["spec"];
                if (spec["capacity"] && spec["capacity"]["storage"]) {
                    volume.capacity = spec["capacity"]["storage"].as<std::string>();
                }
                if (spec["storageClassName"]) {
                    volume.storage_class = spec["storageClassName"].as<std::string>();
                }
                if (spec["accessModes"] && spec["accessModes"].IsSequence() && spec["accessModes"].size() > 0) {
                    volume.access_modes = spec["accessModes"][0].as<std::string>();
                }
            }

            bind_matching_volume(mod, volume);
            mod->get_pvs_mutable().push_back(volume);

            std::ostringstream out;
            out << "persistentvolume/" << name << " created\n";
            if (volume.status == "Bound") {
                out << "persistentvolume/" << name << " bound to " << volume.claim << "\n";
            }
            return out.str();
        }

        return "Error: kubectl create -f supports NetworkPolicy and PersistentVolume (got " +
               (kind.empty() ? "unknown kind" : kind) + ")\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_CREATE_FROM_FILE_H
