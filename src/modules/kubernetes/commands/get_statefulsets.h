#ifndef K8S_COMMANDS_GET_STATEFULSETS_H
#define K8S_COMMANDS_GET_STATEFULSETS_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>

namespace k8s_commands {

inline auto cmd_get_statefulsets(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        std::string wanted;
        for (const auto& arg : args) {
            if (!arg.empty() && arg[0] != '-') {
                wanted = arg;
                break;
            }
        }

        const auto& sets = mod->get_statefulsets();
        bool found = false;
        std::ostringstream rows;
        for (const auto& sts : sets) {
            if (!wanted.empty() && sts.name != wanted) continue;
            found = true;
            std::string ready = std::to_string(sts.ready_replicas) + "/" + std::to_string(sts.replicas);
            rows << std::left
                 << std::setw(24) << sts.name
                 << std::setw(10) << ready
                 << sts.age << "\n";
        }

        if (!wanted.empty() && !found) {
            return "Error from server (NotFound): statefulsets.apps \"" + wanted + "\" not found\n";
        }
        if (!found) {
            return "No resources found in default namespace.\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(24) << "NAME"
            << std::setw(10) << "READY"
            << "AGE\n";
        out << rows.str();
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_STATEFULSETS_H
