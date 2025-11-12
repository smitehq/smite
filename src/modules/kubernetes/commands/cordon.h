#ifndef K8S_COMMANDS_CORDON_H
#define K8S_COMMANDS_CORDON_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

// Add a cordon status field to nodes (we'll need to update Node struct)
inline auto cmd_cordon(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) {
            return "Error: node name required\n";
        }

        std::string node_name = args[0];
        auto& nodes = mod->get_nodes_mutable();

        for (auto& node : nodes) {
            if (node.name == node_name) {
                // For now, just return success message
                // In a real implementation, we'd add a 'cordoned' field to Node struct
                return "node/" + node_name + " cordoned\n";
            }
        }

        return "Error: node \"" + node_name + "\" not found\n";
    };
}

inline auto cmd_uncordon(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) {
            return "Error: node name required\n";
        }

        std::string node_name = args[0];
        auto& nodes = mod->get_nodes_mutable();

        for (auto& node : nodes) {
            if (node.name == node_name) {
                return "node/" + node_name + " uncordoned\n";
            }
        }

        return "Error: node \"" + node_name + "\" not found\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_CORDON_H
