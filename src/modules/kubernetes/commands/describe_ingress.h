#ifndef K8S_COMMANDS_DESCRIBE_INGRESS_H
#define K8S_COMMANDS_DESCRIBE_INGRESS_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

inline std::string describe_one_ingress(const Ingress& ing) {
    std::ostringstream out;
    out << "Name:             " << ing.name << "\n"
        << "Namespace:        " << ing.ns << "\n"
        << "Rules:\n"
        << "  Host                Path  Backend\n"
        << "  ----                ----  -------\n";

    std::string last_host;
    for (const auto& rule : ing.rules) {
        std::string host = rule.host == last_host ? "" : rule.host;
        last_host = rule.host;
        out << "  " << host;
        if (host.size() < 18) out << std::string(18 - host.size(), ' ');
        else out << "  ";
        out << rule.path;
        if (rule.path.size() < 6) out << std::string(6 - rule.path.size(), ' ');
        else out << "  ";
        out << rule.service_name << ":" << rule.service_port << "\n";
    }
    if (ing.rules.empty()) {
        out << "  <none>\n";
    }
    return out.str();
}

inline auto cmd_describe_ingress(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        std::string wanted;
        for (const auto& arg : args) {
            if (!arg.empty() && arg[0] != '-') {
                wanted = arg;
                break;
            }
        }

        const auto& ingresses = mod->get_ingresses();
        if (!wanted.empty()) {
            for (const auto& ing : ingresses) {
                if (ing.name == wanted) return describe_one_ingress(ing);
            }
            return "Error from server (NotFound): ingresses.networking.k8s.io \"" + wanted + "\" not found\n";
        }

        if (ingresses.empty()) return "No resources found in default namespace.\n";

        std::ostringstream out;
        for (size_t i = 0; i < ingresses.size(); ++i) {
            if (i) out << "\n";
            out << describe_one_ingress(ingresses[i]);
        }
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_DESCRIBE_INGRESS_H
