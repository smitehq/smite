#ifndef K8S_COMMANDS_GET_CONFIGMAPS_H
#define K8S_COMMANDS_GET_CONFIGMAPS_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>

namespace k8s_commands {

inline auto cmd_get_configmaps(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        std::ostringstream out;

        const auto& configmaps = mod->get_configmaps();

        if (configmaps.empty()) {
            out << "No resources found in default namespace.\n";
            return out.str();
        }

        // Header
        out << std::left
            << std::setw(35) << "NAME"
            << std::setw(10) << "DATA"
            << std::setw(8) << "AGE"
            << "\n";

        for (const auto& cm : configmaps) {
            out << std::left
                << std::setw(35) << cm.name
                << std::setw(10) << cm.data.size()
                << std::setw(8) << cm.age
                << "\n";
        }

        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_CONFIGMAPS_H
