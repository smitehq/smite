#ifndef K8S_COMMANDS_CREATE_CONFIGMAP_H
#define K8S_COMMANDS_CREATE_CONFIGMAP_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

inline auto cmd_create_configmap(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string {
        // kubectl create configmap <name> --from-literal=key=value
        // or kubectl create configmap <name> --from-file=path

        if (args.empty()) {
            return "Error: configmap name required\n";
        }

        std::string name = args[0];
        std::unordered_map<std::string, std::string> data;

        // Parse --from-literal flags
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i].find("--from-literal=") == 0) {
                std::string kv = args[i].substr(15);
                size_t eq_pos = kv.find('=');
                if (eq_pos != std::string::npos) {
                    std::string key = kv.substr(0, eq_pos);
                    std::string value = kv.substr(eq_pos + 1);
                    data[key] = value;
                }
            }
        }

        ConfigMap cm;
        cm.name = name;
        cm.ns = "default";
        cm.data = data;
        cm.age = "0s";

        module->add_configmap(cm);

        return "configmap/" + name + " created\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_CREATE_CONFIGMAP_H
