#ifndef K8S_COMMANDS_GET_NETWORKPOLICIES_H
#define K8S_COMMANDS_GET_NETWORKPOLICIES_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>

namespace k8s_commands {

inline auto cmd_get_networkpolicies(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>&) -> std::string {
        const auto& policies = mod->get_network_policies();
        if (policies.empty()) {
            return "No resources found in default namespace.\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(32) << "NAME"
            << std::setw(16) << "POD-SELECTOR"
            << "AGE\n";
        for (const auto& policy : policies) {
            out << std::left
                << std::setw(32) << policy.name
                << std::setw(16) << "<none>"
                << policy.age << "\n";
        }
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_NETWORKPOLICIES_H
