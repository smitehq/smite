#ifndef K8S_COMMANDS_ROLLOUT_H
#define K8S_COMMANDS_ROLLOUT_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

// Quests say `kubectl rollout <verb> deployment <name>`. The prefix is only
// "kubectl rollout", so both the resource type and the name remain in args.
// `deployment/<name>` is accepted as well.
inline bool rollout_target(const std::vector<std::string>& args, std::string& name, std::string& error) {
    const std::string usage =
        "Usage: kubectl rollout " + (args.empty() ? "<status|history|undo>" : args[0]) +
        " deployment <name>\n";

    if (args.size() < 2) {
        error = "Error: resource name required\n" + usage;
        return false;
    }

    const std::string& resource = args[1];
    if (resource == "deployment" || resource == "deploy") {
        if (args.size() < 3 || args[2].empty() || args[2][0] == '-') {
            error = "Error: resource name required\n" + usage;
            return false;
        }
        name = args[2];
        return true;
    }

    const auto slash = resource.find('/');
    if (slash != std::string::npos) {
        const std::string kind = resource.substr(0, slash);
        name = resource.substr(slash + 1);
        if ((kind != "deployment" && kind != "deploy") || name.empty()) {
            error = "Error: rollout is only supported for deployments\n" + usage;
            return false;
        }
        return true;
    }

    name = resource;
    return true;
}

inline auto cmd_rollout(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) {
            return "Error: rollout requires a subcommand (status, undo, history)\n"
                   "Usage: kubectl rollout status deployment <name>\n";
        }

        const std::string subcommand = args[0];
        if (subcommand != "status" && subcommand != "undo" && subcommand != "history") {
            return "Error: unknown rollout subcommand: " + subcommand + "\n";
        }

        std::string name;
        std::string error;
        if (!rollout_target(args, name, error)) return error;

        auto& deployments = mod->get_deployments_mutable();
        Deployment* found = nullptr;
        for (auto& dep : deployments) {
            if (dep.name == name) {
                found = &dep;
                break;
            }
        }
        if (!found) {
            return "Error: deployment \"" + name + "\" not found\n";
        }

        if (subcommand == "status") {
            std::ostringstream out;
            if (found->ready_replicas == found->replicas) {
                out << "deployment \"" << name << "\" successfully rolled out\n";
            } else {
                out << "Waiting for deployment \"" << name << "\" rollout to finish: "
                    << found->ready_replicas << " of " << found->replicas
                    << " updated replicas are available...\n";
            }
            return out.str();
        }

        if (subcommand == "history") {
            std::ostringstream out;
            out << "deployment.apps/" << name << "\n";
            out << "REVISION  CHANGE-CAUSE\n";
            const int revision = found->revision < 1 ? 1 : found->revision;
            for (int i = 1; i <= revision; ++i) {
                out << i << "         <none>\n";
            }
            return out.str();
        }

        const std::string old_image = found->image;
        const auto broken = found->image.rfind("-broken");
        if (broken != std::string::npos) {
            found->image.erase(broken, std::string("-broken").size());
        }
        if (found->revision > 1) found->revision--;
        found->ready_replicas = found->replicas;
        found->available_replicas = found->replicas;

        for (auto& node : mod->get_nodes_mutable()) {
            for (auto& pod : node.pods) {
                if (pod.image != old_image) continue;
                pod.image = found->image;
                pod.status = "Running";
                pod.container_state = "running";
                pod.restarts = 0;
            }
        }

        return "deployment.apps/" + name + " rolled back\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_ROLLOUT_H
