#ifndef K8S_COMMANDS_GET_RESOURCEQUOTA_H
#define K8S_COMMANDS_GET_RESOURCEQUOTA_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>

namespace k8s_commands {

inline auto cmd_get_resourcequota(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>&) -> std::string {
        const auto& quotas = mod->get_resource_quotas();
        if (quotas.empty()) {
            return "No resources found in default namespace.\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(24) << "NAME"
            << std::setw(10) << "AGE"
            << "REQUEST\n";

        for (const auto& quota : quotas) {
            std::vector<std::string> resources;
            for (const auto& [key, hard] : quota.hard) {
                std::string used = "0";
                auto it = quota.used.find(key);
                if (it != quota.used.end()) used = it->second;
                resources.push_back(key + ": " + used + "/" + hard);
            }
            std::sort(resources.begin(), resources.end());

            std::string request;
            for (size_t i = 0; i < resources.size(); ++i) {
                if (i) request += ", ";
                request += resources[i];
            }
            if (request.empty()) request = "<none>";

            out << std::left
                << std::setw(24) << quota.name
                << std::setw(10) << quota.age
                << request << "\n";
        }
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_RESOURCEQUOTA_H
