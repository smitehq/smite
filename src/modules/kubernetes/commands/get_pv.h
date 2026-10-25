#ifndef K8S_COMMANDS_GET_PV_H
#define K8S_COMMANDS_GET_PV_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>

namespace k8s_commands {

inline auto cmd_get_pv(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>&) -> std::string {
        const auto& pvs = mod->get_pvs();
        if (pvs.empty()) {
            return "No resources found\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(20) << "NAME"
            << std::setw(12) << "CAPACITY"
            << std::setw(16) << "ACCESS MODES"
            << std::setw(12) << "STATUS"
            << std::setw(18) << "CLAIM"
            << std::setw(16) << "STORAGECLASS"
            << "AGE\n";

        for (const auto& pv : pvs) {
            out << std::left
                << std::setw(20) << pv.name
                << std::setw(12) << pv.capacity
                << std::setw(16) << pv.access_modes
                << std::setw(12) << pv.status
                << std::setw(18) << (pv.claim.empty() ? "<none>" : pv.claim)
                << std::setw(16) << pv.storage_class
                << pv.age << "\n";
        }
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_PV_H
