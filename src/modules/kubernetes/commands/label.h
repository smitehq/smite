#ifndef K8S_COMMANDS_LABEL_H
#define K8S_COMMANDS_LABEL_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

inline auto cmd_label_node(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // Prefix "kubectl label node" is stripped, so args are:
        // <node> <key>=<value> [<key>=<value>...]
        const std::string usage =
            "Usage: kubectl label node <name> <key>=<value>\n";

        if (args.empty() || args[0].empty() || args[0][0] == '-') {
            return "Error: node name required\n" + usage;
        }

        const std::string node_name = args[0];
        bool saw_label = false;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].empty() || args[i][0] == '-') continue;
            if (args[i].find('=') == std::string::npos) {
                return "Error: labels must be key=value\n" + usage;
            }
            saw_label = true;
        }
        if (!saw_label) {
            return "Error: at least one key=value label is required\n" + usage;
        }

        auto& nodes = mod->get_nodes_mutable();
        for (auto& node : nodes) {
            if (node.name != node_name) continue;
            for (size_t i = 1; i < args.size(); ++i) {
                if (args[i].empty() || args[i][0] == '-') continue;
                auto eq = args[i].find('=');
                node.labels[args[i].substr(0, eq)] = args[i].substr(eq + 1);
            }
            return "node/" + node_name + " labeled\n";
        }

        return "Error: node \"" + node_name + "\" not found\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_LABEL_H
