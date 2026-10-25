#ifndef K8S_COMMANDS_SCALE_H
#define K8S_COMMANDS_SCALE_H

#include "modules/kubernetes/module.h"
#include <string>

namespace k8s_commands {

inline auto cmd_scale(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // Registered prefixes are "kubectl scale deployment" and "kubectl scale deploy".
        // The router strips that prefix, so args start at the deployment name:
        // <name> --replicas=<count> or <name> --replicas <count>.

        const std::string usage =
            "Usage: kubectl scale deployment <name> --replicas=<count>\n";

        if (args.empty() || args[0].empty() || args[0][0] == '-') {
            return "Error: kubectl scale requires a deployment name and --replicas flag\n" + usage;
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

        auto& deployments = mod->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == resource_name) {
                dep.replicas = new_replicas;
                dep.ready_replicas = new_replicas;
                dep.available_replicas = new_replicas;
                return "deployment.apps/" + resource_name + " scaled\n";
            }
        }

        return "Error: deployment \"" + resource_name + "\" not found\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_SCALE_H
