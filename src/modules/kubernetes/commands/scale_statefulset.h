#ifndef K8S_COMMANDS_SCALE_STATEFULSET_H
#define K8S_COMMANDS_SCALE_STATEFULSET_H

#include "modules/kubernetes/module.h"
#include <algorithm>
#include <string>

namespace k8s_commands {

inline auto cmd_scale_statefulset(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // Registered prefixes are "kubectl scale statefulset" and "kubectl scale sts".
        // The router strips that prefix, so args start at the statefulset name.
        const std::string usage =
            "Usage: kubectl scale statefulset <name> --replicas=<count>\n";

        if (args.empty() || args[0].empty() || args[0][0] == '-') {
            return "Error: kubectl scale requires a statefulset name and --replicas flag\n" + usage;
        }

        const std::string resource_name = args[0];
        int new_replicas = 0;
        bool found_replicas = false;

        auto read_replicas = [](const std::string& value, int& out) -> bool {
            if (value.empty()) return false;
            try {
                size_t consumed = 0;
                const int parsed = std::stoi(value, &consumed);
                if (consumed != value.size() || parsed < 0) return false;
                out = parsed;
                return true;
            } catch (...) {
                return false;
            }
        };

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].find("--replicas=") == 0) {
                const std::string value = args[i].substr(std::string("--replicas=").size());
                if (!read_replicas(value, new_replicas)) {
                    return "Error: Invalid replicas value\n";
                }
                found_replicas = true;
            } else if (args[i] == "--replicas") {
                if (i + 1 >= args.size() || !read_replicas(args[i + 1], new_replicas)) {
                    return "Error: Invalid replicas value\n";
                }
                ++i;
                found_replicas = true;
            }
        }

        if (!found_replicas) {
            return "Error: --replicas flag is required\n" + usage;
        }

        auto& sets = mod->get_statefulsets_mutable();
        StatefulSet* sts = nullptr;
        for (auto& candidate : sets) {
            if (candidate.name == resource_name) {
                sts = &candidate;
                break;
            }
        }
        if (!sts) {
            return "Error: statefulset \"" + resource_name + "\" not found\n";
        }

        auto pod_named = [&](const std::string& pod_name) -> Pod* {
            for (auto& node : mod->get_nodes_mutable()) {
                for (auto& pod : node.pods) {
                    if (pod.name == pod_name) return &pod;
                }
            }
            return nullptr;
        };

        auto& nodes = mod->get_nodes_mutable();
        for (int ordinal = 0; ordinal < new_replicas; ++ordinal) {
            const std::string pod_name = resource_name + "-" + std::to_string(ordinal);
            if (pod_named(pod_name) != nullptr) continue;
            if (nodes.empty()) break;

            Pod pod;
            pod.name = pod_name;
            pod.status = "Running";
            pod.image = sts->image;
            pod.container_state = "running";
            pod.labels["app"] = resource_name;
            nodes[static_cast<size_t>(ordinal) % nodes.size()].pods.push_back(pod);
        }

        for (int ordinal = new_replicas;; ++ordinal) {
            const std::string pod_name = resource_name + "-" + std::to_string(ordinal);
            bool removed = false;
            for (auto& node : nodes) {
                auto before = node.pods.size();
                node.pods.erase(
                    std::remove_if(node.pods.begin(), node.pods.end(),
                        [&](const Pod& pod) { return pod.name == pod_name; }),
                    node.pods.end());
                if (node.pods.size() != before) removed = true;
            }
            if (!removed) break;
        }

        sts->replicas = new_replicas;
        sts->ready_replicas = new_replicas;
        return "statefulset.apps/" + resource_name + " scaled\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_SCALE_STATEFULSET_H
