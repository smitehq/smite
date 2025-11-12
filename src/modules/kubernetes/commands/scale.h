#ifndef K8S_COMMANDS_SCALE_H
#define K8S_COMMANDS_SCALE_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <string>

namespace k8s_commands {

inline auto cmd_scale(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // kubectl scale deployment <name> --replicas=<count>

        if (args.size() < 3) {
            return "Error: kubectl scale requires resource type, name, and --replicas flag\n"
                   "Usage: kubectl scale deployment <name> --replicas=<count>\n";
        }

        std::string resource_type = args[0];
        std::string resource_name = args[1];

        // Parse --replicas=N
        int new_replicas = 0;
        bool found_replicas = false;
        for (size_t i = 2; i < args.size(); ++i) {
            if (args[i].find("--replicas=") == 0) {
                try {
                    new_replicas = std::stoi(args[i].substr(11));
                    found_replicas = true;
                } catch (...) {
                    return "Error: Invalid replicas value\n";
                }
            }
        }

        if (!found_replicas) {
            return "Error: --replicas flag is required\n";
        }

        if (resource_type != "deployment" && resource_type != "deploy") {
            return "Error: scaling is only supported for deployments\n";
        }

        // Find and scale the deployment
        auto& deployments = mod->get_deployments_mutable();
        bool found = false;
        for (auto& dep : deployments) {
            if (dep.name == resource_name) {
                dep.replicas = new_replicas;
                dep.ready_replicas = new_replicas;
                dep.available_replicas = new_replicas;
                found = true;
                break;
            }
        }

        if (!found) {
            return "Error: deployment \"" + resource_name + "\" not found\n";
        }

        return "deployment.apps/" + resource_name + " scaled\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_SCALE_H
