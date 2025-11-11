#ifndef MODULES_KUBERNETES_COMMANDS_GET_NODES_H
#define MODULES_KUBERNETES_COMMANDS_GET_NODES_H

#include "../module.h"
#include <sstream>
#include <iomanip>

namespace kubectl_commands {

inline auto cmd_get_nodes(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        std::ostringstream out;

        out << std::left << std::setw(10) << "NAME" << std::setw(12) << "STATUS" << "ROLES\n";
        for (const auto& n : module->get_nodes()) {
            std::string status = "Ready"; // assume all nodes ready
            std::string roles = "worker"; // default role
            out << std::setw(10) << n.name << std::setw(12) << status << roles << "\n";
        }
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_GET_NODES_H
