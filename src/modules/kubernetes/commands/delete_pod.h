#ifndef MODULES_KUBERNETES_COMMANDS_DELETE_POD_H
#define MODULES_KUBERNETES_COMMANDS_DELETE_POD_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_delete_pod(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) {
            return "Error: pod name required\n";
        }

        std::string pod_name = args[0];

        // Find and remove the pod
        for (auto& node : module->get_nodes_mutable()) {
            auto it = std::find_if(node.pods.begin(), node.pods.end(),
                [&](const Pod& p) { return p.name == pod_name; });

            if (it != node.pods.end()) {
                // Check if the pod should come back (simulating deployment controller)
                // In a real cluster, the deployment would recreate the pod
                // For now, we'll just mark it as deleted
                node.pods.erase(it);
                return "pod \"" + pod_name + "\" deleted\n";
            }
        }

        return "Error from server (NotFound): pods \"" + pod_name + "\" not found\n";
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_DELETE_POD_H
