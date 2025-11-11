#ifndef MODULES_KUBERNETES_COMMANDS_TOP_PODS_H
#define MODULES_KUBERNETES_COMMANDS_TOP_PODS_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_top_pods(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        std::ostringstream out;
        size_t name_width = 4;

        for (const auto& node : module->get_nodes())
            for (const auto& p : node.pods)
                name_width = std::max(name_width, p.name.size());

        out << std::left
            << std::setw(name_width + 2) << "NAME"
            << std::setw(8) << "CPU(%)"
            << "MEM(%)\n";

        for (const auto& node : module->get_nodes()) {
            for (const auto& p : node.pods) {
                int cpu = module->random_usage(50);  // simulate pod CPU usage 1-50%
                int mem = module->random_usage(50);  // simulate pod MEM usage 1-50%
                out << std::setw(name_width + 2) << p.name
                    << std::setw(8) << cpu
                    << mem << "\n";
            }
        }
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_TOP_PODS_H
