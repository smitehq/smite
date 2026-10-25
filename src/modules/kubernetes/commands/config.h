#ifndef K8S_COMMANDS_CONFIG_H
#define K8S_COMMANDS_CONFIG_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>

namespace k8s_commands {

inline auto cmd_config_current_context(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>&) -> std::string {
        if (mod->get_current_context().empty()) {
            return "error: current-context is not set\n";
        }
        return mod->get_current_context() + "\n";
    };
}

inline auto cmd_config_get_contexts(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>&) -> std::string {
        const auto& contexts = mod->get_contexts();
        if (contexts.empty()) {
            return "error: current-context is not set\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(12) << "CURRENT"
            << std::setw(20) << "NAME"
            << "CLUSTER\n";
        for (const auto& ctx : contexts) {
            std::string marker = ctx.name == mod->get_current_context() ? "*" : "";
            out << std::left
                << std::setw(12) << marker
                << std::setw(20) << ctx.name
                << ctx.cluster << "\n";
        }
        return out.str();
    };
}

inline auto cmd_config_use_context(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        if (args.empty() || args[0].empty()) {
            return "Error: context name required\n"
                   "Usage: kubectl config use-context <name>\n";
        }
        if (!mod->use_context(args[0])) {
            return "error: no context exists with the name: \"" + args[0] + "\"\n";
        }
        return "Switched to context \"" + args[0] + "\".\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_CONFIG_H
