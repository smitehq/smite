#ifndef MODULES_KUBERNETES_COMMANDS_DESCRIBE_NODE_H
#define MODULES_KUBERNETES_COMMANDS_DESCRIBE_NODE_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace kubectl_commands {

inline auto cmd_describe_node(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) return "Error: node name required\n";

        auto it = module->find_node(args[0]);
        if (it == module->get_nodes().end()) return "Error from server (NotFound): node not found\n";

        std::ostringstream out;
        out << "Name: " << it->name << "\nIP: " << it->ip << "\n"
            << "Pods:\n";

        for (const auto& p : it->pods)
            out << "  " << p.name << " (" << p.status << ")\n";

        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_DESCRIBE_NODE_H
