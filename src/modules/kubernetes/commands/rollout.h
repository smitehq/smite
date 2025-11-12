#ifndef K8S_COMMANDS_ROLLOUT_H
#define K8S_COMMANDS_ROLLOUT_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

inline auto cmd_rollout(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // kubectl rollout <subcommand> deployment/<name>

        if (args.empty()) {
            return "Error: rollout requires a subcommand (status, undo, history)\n";
        }

        std::string subcommand = args[0];

        if (subcommand == "status") {
            if (args.size() < 2) {
                return "Error: resource name required\n";
            }

            std::string resource = args[1];
            // Parse deployment/name or just name
            std::string name = resource;
            if (resource.find("deployment/") == 0) {
                name = resource.substr(11);
            }

            const auto& deployments = mod->get_deployments();
            for (const auto& dep : deployments) {
                if (dep.name == name) {
                    std::ostringstream out;
                    if (dep.ready_replicas == dep.replicas) {
                        out << "deployment \"" << name << "\" successfully rolled out\n";
                    } else {
                        out << "Waiting for deployment \"" << name << "\" rollout to finish: "
                            << dep.ready_replicas << " of " << dep.replicas << " updated replicas are available...\n";
                    }
                    return out.str();
                }
            }
            return "Error: deployment \"" + name + "\" not found\n";

        } else if (subcommand == "undo") {
            if (args.size() < 2) {
                return "Error: resource name required\n";
            }

            std::string resource = args[1];
            std::string name = resource;
            if (resource.find("deployment/") == 0) {
                name = resource.substr(11);
            }

            auto& deployments = mod->get_deployments_mutable();
            for (auto& dep : deployments) {
                if (dep.name == name) {
                    // Simulate rollback
                    if (dep.revision > 1) {
                        dep.revision--;
                    }
                    return "deployment.apps/" + name + " rolled back\n";
                }
            }
            return "Error: deployment \"" + name + "\" not found\n";

        } else if (subcommand == "history") {
            if (args.size() < 2) {
                return "Error: resource name required\n";
            }

            std::string resource = args[1];
            std::string name = resource;
            if (resource.find("deployment/") == 0) {
                name = resource.substr(11);
            }

            const auto& deployments = mod->get_deployments();
            for (const auto& dep : deployments) {
                if (dep.name == name) {
                    std::ostringstream out;
                    out << "deployment.apps/" << name << "\n";
                    out << "REVISION  CHANGE-CAUSE\n";
                    for (int i = 1; i <= dep.revision; ++i) {
                        out << i << "         <none>\n";
                    }
                    return out.str();
                }
            }
            return "Error: deployment \"" + name + "\" not found\n";

        } else {
            return "Error: unknown rollout subcommand: " + subcommand + "\n";
        }
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_ROLLOUT_H
