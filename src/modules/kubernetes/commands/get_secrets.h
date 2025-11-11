#ifndef MODULES_KUBERNETES_COMMANDS_GET_SECRETS_H
#define MODULES_KUBERNETES_COMMANDS_GET_SECRETS_H

#include "../module.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_get_secrets(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
    std::ostringstream out;
    size_t name_width = 4;
    size_t type_width = 4;

    for (const auto& s : module->get_secrets()) {
        name_width = std::max(name_width, s.name.size());
        type_width = std::max(type_width, s.type.size());
    }

    out << std::left
        << std::setw(name_width + 2) << "NAME"
        << std::setw(type_width + 2) << "TYPE"
        << std::setw(6) << "DATA"
        << "AGE\n";

    for (const auto& s : module->get_secrets()) {
        out << std::setw(name_width + 2) << s.name
            << std::setw(type_width + 2) << s.type
            << std::setw(6) << s.data.size()
            << s.age << "\n";
    }

    return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_GET_SECRETS_H
