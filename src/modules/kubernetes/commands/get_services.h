#ifndef K8S_COMMANDS_GET_SERVICES_H
#define K8S_COMMANDS_GET_SERVICES_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>

namespace k8s_commands {

inline auto cmd_get_services(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        std::ostringstream out;

        const auto& services = mod->get_services();

        if (services.empty()) {
            out << "No resources found in default namespace.\n";
            return out.str();
        }

        // Header
        out << std::left
            << std::setw(25) << "NAME"
            << std::setw(15) << "TYPE"
            << std::setw(18) << "CLUSTER-IP"
            << std::setw(18) << "EXTERNAL-IP"
            << std::setw(20) << "PORT(S)"
            << std::setw(8) << "AGE"
            << "\n";

        for (const auto& svc : services) {
            std::string ports_str;
            for (size_t i = 0; i < svc.ports.size(); ++i) {
                if (i > 0) ports_str += ",";
                ports_str += svc.ports[i];
            }

            out << std::left
                << std::setw(25) << svc.name
                << std::setw(15) << svc.type
                << std::setw(18) << svc.cluster_ip
                << std::setw(18) << svc.external_ip
                << std::setw(20) << ports_str
                << std::setw(8) << svc.age
                << "\n";
        }

        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_SERVICES_H
