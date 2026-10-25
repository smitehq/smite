#ifndef K8S_COMMANDS_DELETE_DEPLOYMENT_H
#define K8S_COMMANDS_DELETE_DEPLOYMENT_H

#include "modules/kubernetes/module.h"
#include <algorithm>

namespace k8s_commands {

inline auto cmd_delete_deployment(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // Prefix "kubectl delete deployment" (or "deploy") is stripped.
        if (args.empty() || args[0].empty() || args[0][0] == '-') {
            return "Error: deployment name required\n"
                   "Usage: kubectl delete deployment <name>\n";
        }

        const std::string name = args[0];
        auto& deployments = mod->get_deployments_mutable();
        auto it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == name; });
        if (it == deployments.end()) {
            return "Error from server (NotFound): deployments.apps \"" + name + "\" not found\n";
        }

        deployments.erase(it);

        const std::string prefix = name + "-";
        for (auto& node : mod->get_nodes_mutable()) {
            node.pods.erase(
                std::remove_if(node.pods.begin(), node.pods.end(),
                    [&](const Pod& pod) {
                        return pod.name == name || pod.name.rfind(prefix, 0) == 0;
                    }),
                node.pods.end());
        }

        return "deployment.apps \"" + name + "\" deleted\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_DELETE_DEPLOYMENT_H
