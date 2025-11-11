#ifndef MODULES_KUBERNETES_COMMANDS_LOGS_H
#define MODULES_KUBERNETES_COMMANDS_LOGS_H

#include "../module.h"
#include <sstream>

namespace kubectl_commands {

inline auto cmd_logs(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) return "Error: pod name required\n";

        auto it = module->find_pod(args[0]);
        if (it == decltype(module->get_nodes()[0].pods.begin()){ }) {
            std::ostringstream out;
            out << "Error from server (NotFound): pod \"" << args[0] << "\" not found\n";
            for (const auto& node : module->get_nodes())
                for (const auto& p : node.pods)
                    if (p.name.rfind(args[0], 0) == 0)
                        out << "Did you mean \"" << p.name << "\"?\n";
            return out.str();
        }

        std::ostringstream out;
        const Pod& pod = *it;
        for (const auto& log : pod.logs) {
            out << log.timestamp << " " << pod.name << ": " << log.message << "\n";
        }
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_LOGS_H
