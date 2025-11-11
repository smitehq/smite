#ifndef MODULES_KUBERNETES_COMMANDS_TOP_NODES_H
#define MODULES_KUBERNETES_COMMANDS_TOP_NODES_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_top_nodes(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        std::ostringstream out;
        size_t name_width = 4; // NAME column

        for (const auto& n : module->get_nodes())
            name_width = std::max(name_width, n.name.size());

        out << std::left
            << std::setw(name_width + 2) << "NAME"
            << std::setw(8) << "CPU(%)"
            << "MEM(%)\n";

        for (const auto& n : module->get_nodes()) {
            int cpu = module->random_usage(80);  // simulate CPU usage 1-80%
            int mem = module->random_usage(70);  // simulate Memory usage 1-70%
            out << std::setw(name_width + 2) << n.name
                << std::setw(8) << cpu
                << mem << "\n";
        }
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_TOP_NODES_H
