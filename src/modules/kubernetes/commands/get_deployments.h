#ifndef K8S_COMMANDS_GET_DEPLOYMENTS_H
#define K8S_COMMANDS_GET_DEPLOYMENTS_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>

namespace k8s_commands {

inline auto cmd_get_deployments(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        std::ostringstream out;

        const auto& deployments = mod->get_deployments();

        if (deployments.empty()) {
            out << "No resources found in default namespace.\n";
            return out.str();
        }

        // Header
        out << std::left
            << std::setw(30) << "NAME"
            << std::setw(10) << "READY"
            << std::setw(12) << "UP-TO-DATE"
            << std::setw(12) << "AVAILABLE"
            << std::setw(8) << "AGE"
            << "\n";

        for (const auto& dep : deployments) {
            std::string ready = std::to_string(dep.ready_replicas) + "/" + std::to_string(dep.replicas);
            out << std::left
                << std::setw(30) << dep.name
                << std::setw(10) << ready
                << std::setw(12) << dep.ready_replicas
                << std::setw(12) << dep.available_replicas
                << std::setw(8) << dep.age
                << "\n";
        }

        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_DEPLOYMENTS_H
