#ifndef K8S_COMMANDS_GET_PVC_H
#define K8S_COMMANDS_GET_PVC_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>

namespace k8s_commands {

inline auto cmd_get_pvc(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>&) -> std::string {
        const auto& pvcs = mod->get_pvcs();
        if (pvcs.empty()) {
            return "No resources found in default namespace.\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(20) << "NAME"
            << std::setw(12) << "STATUS"
            << std::setw(18) << "VOLUME"
            << std::setw(12) << "CAPACITY"
            << std::setw(16) << "STORAGECLASS"
            << "AGE\n";

        for (const auto& pvc : pvcs) {
            out << std::left
                << std::setw(20) << pvc.name
                << std::setw(12) << pvc.status
                << std::setw(18) << (pvc.volume_name.empty() ? "<none>" : pvc.volume_name)
                << std::setw(12) << pvc.storage_request
                << std::setw(16) << pvc.storage_class
                << pvc.age << "\n";
        }
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_PVC_H
