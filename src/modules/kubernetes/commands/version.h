#ifndef MODULES_KUBERNETES_COMMANDS_VERSION_H
#define MODULES_KUBERNETES_COMMANDS_VERSION_H

#include "../module.h"
#include <sstream>
#include <functional>

namespace kubectl_commands {

inline auto cmd_version(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string {
        std::ostringstream out;
        out << "Client Version: v1.28.0\n";
        out << "Server Version: v1.28.0 (simulated)\n";
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_VERSION_H
