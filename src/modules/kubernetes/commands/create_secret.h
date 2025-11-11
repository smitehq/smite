#ifndef MODULES_KUBERNETES_COMMANDS_CREATE_SECRET_H
#define MODULES_KUBERNETES_COMMANDS_CREATE_SECRET_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace kubectl_commands {

inline auto cmd_create_secret_generic(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) {
            return "Error: secret name required\n";
        }

        std::string secret_name = args[0];

        // Check if secret already exists
        auto it = module->find_secret(secret_name);
        if (it != module->secret_end()) {
            return "Error from server (AlreadyExists): secrets \"" + secret_name + "\" already exists\n";
        }

        // Parse --from-literal arguments
        Secret new_secret;
        new_secret.name = secret_name;
        new_secret.type = "Opaque";
        new_secret.age = "0s";

        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i] == "--from-literal" && i + 1 < args.size()) {
                std::string literal = args[i + 1];
                size_t eq_pos = literal.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = literal.substr(0, eq_pos);
                    std::string value = literal.substr(eq_pos + 1);
                    new_secret.data[key] = value;
                }
                ++i;
            }
        }

        module->add_secret(new_secret);

        std::ostringstream output;
        output << "secret/" << secret_name << " created\n";

        // Check if this creates a secret that a quest is waiting for
        std::string completion_msg = module->check_secret_trigger(secret_name);
        if (!completion_msg.empty()) {
            output << "\n" << completion_msg << "\n";
        }

        return output.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_CREATE_SECRET_H
