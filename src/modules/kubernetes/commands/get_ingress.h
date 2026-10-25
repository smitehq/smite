#ifndef K8S_COMMANDS_GET_INGRESS_H
#define K8S_COMMANDS_GET_INGRESS_H

#include "modules/kubernetes/module.h"
#include <iomanip>
#include <sstream>
#include <set>

namespace k8s_commands {

inline std::string ingress_to_yaml(const Ingress& ing) {
    std::ostringstream yaml;
    yaml << "apiVersion: networking.k8s.io/v1\n"
         << "kind: Ingress\n"
         << "metadata:\n"
         << "  name: " << ing.name << "\n"
         << "  namespace: " << ing.ns << "\n"
         << "spec:\n"
         << "  rules:\n";

    for (size_t i = 0; i < ing.rules.size();) {
        const std::string host = ing.rules[i].host;
        yaml << "  - host: " << host << "\n"
             << "    http:\n"
             << "      paths:\n";
        while (i < ing.rules.size() && ing.rules[i].host == host) {
            const auto& rule = ing.rules[i];
            yaml << "      - path: " << rule.path << "\n"
                 << "        pathType: Prefix\n"
                 << "        backend:\n"
                 << "          service:\n"
                 << "            name: " << rule.service_name << "\n"
                 << "            port:\n"
                 << "              number: " << rule.service_port << "\n";
            ++i;
        }
    }
    return yaml.str();
}

inline auto cmd_get_ingress(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        bool yaml_out = false;
        std::string wanted;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "-o" && i + 1 < args.size()) {
                yaml_out = args[++i] == "yaml";
            } else if (args[i].rfind("-o=", 0) == 0) {
                yaml_out = args[i].substr(3) == "yaml";
            } else if (!args[i].empty() && args[i][0] != '-') {
                wanted = args[i];
            }
        }

        const auto& ingresses = mod->get_ingresses();
        const Ingress* match = nullptr;
        if (!wanted.empty()) {
            for (const auto& ing : ingresses) {
                if (ing.name == wanted) {
                    match = &ing;
                    break;
                }
            }
            if (!match) {
                return "Error from server (NotFound): ingresses.networking.k8s.io \"" + wanted + "\" not found\n";
            }
        }

        if (yaml_out) {
            if (match) return ingress_to_yaml(*match);
            if (ingresses.empty()) return "No resources found in default namespace.\n";
            std::ostringstream out;
            for (size_t i = 0; i < ingresses.size(); ++i) {
                if (i) out << "---\n";
                out << ingress_to_yaml(ingresses[i]);
            }
            return out.str();
        }

        if (ingresses.empty()) {
            return "No resources found in default namespace.\n";
        }

        std::ostringstream out;
        out << std::left
            << std::setw(20) << "NAME"
            << std::setw(12) << "CLASS"
            << std::setw(40) << "HOSTS"
            << std::setw(10) << "PORTS"
            << "AGE\n";

        for (const auto& ing : ingresses) {
            if (match && ing.name != match->name) continue;
            std::set<std::string> hosts;
            for (const auto& rule : ing.rules) {
                if (!rule.host.empty()) hosts.insert(rule.host);
            }
            std::string host_list;
            for (const auto& host : hosts) {
                if (!host_list.empty()) host_list += ",";
                host_list += host;
            }
            if (host_list.empty()) host_list = "*";

            out << std::left
                << std::setw(20) << ing.name
                << std::setw(12) << "<none>"
                << std::setw(40) << host_list
                << std::setw(10) << "80"
                << (ing.age.empty() ? "0s" : ing.age) << "\n";
        }
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_GET_INGRESS_H
