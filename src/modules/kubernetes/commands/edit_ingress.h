#ifndef K8S_COMMANDS_EDIT_INGRESS_H
#define K8S_COMMANDS_EDIT_INGRESS_H

#include "modules/kubernetes/module.h"
#include "modules/kubernetes/commands/get_ingress.h"
#include "shell/nano.h"
#include <cstdlib>
#include <yaml-cpp/yaml.h>

namespace k8s_commands {

inline bool apply_ingress_yaml(Ingress& ing, const std::string& edited_yaml, std::string& error) {
    YAML::Node edited;
    try {
        edited = YAML::Load(edited_yaml);
    } catch (const YAML::Exception& ex) {
        error = "Error parsing YAML: " + std::string(ex.what()) + "\n";
        return false;
    }

    if (!edited["spec"] || !edited["spec"]["rules"] || !edited["spec"]["rules"].IsSequence()) {
        error = "Error: ingress spec.rules is required\n";
        return false;
    }

    std::vector<IngressRule> rules;
    for (const auto& rule_node : edited["spec"]["rules"]) {
        std::string host;
        if (rule_node["host"]) host = rule_node["host"].as<std::string>();
        if (!rule_node["http"] || !rule_node["http"]["paths"]) continue;

        for (const auto& path_node : rule_node["http"]["paths"]) {
            IngressRule rule;
            rule.host = host;
            if (path_node["path"]) rule.path = path_node["path"].as<std::string>();
            if (path_node["backend"] && path_node["backend"]["service"]) {
                const auto service = path_node["backend"]["service"];
                if (service["name"]) rule.service_name = service["name"].as<std::string>();
                if (service["port"] && service["port"]["number"]) {
                    rule.service_port = service["port"]["number"].as<int>();
                }
            }
            if (!rule.service_name.empty()) rules.push_back(rule);
        }
    }

    if (rules.empty()) {
        error = "Error: ingress must contain at least one backend rule\n";
        return false;
    }

    ing.rules = std::move(rules);
    return true;
}

inline auto cmd_edit_ingress(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        if (args.empty() || args[0].empty() || args[0][0] == '-') {
            return "Error: ingress name required\n"
                   "Usage: kubectl edit ingress <name>\n";
        }

        const std::string name = args[0];
        auto& ingresses = mod->get_ingresses_mutable();
        auto it = std::find_if(ingresses.begin(), ingresses.end(),
            [&](const Ingress& ing) { return ing.name == name; });
        if (it == ingresses.end()) {
            return "Error from server (NotFound): ingresses.networking.k8s.io \"" + name + "\" not found\n";
        }

        const std::string original = ingress_to_yaml(*it);
        std::string edited = original;

        // Tests inject the saved buffer. Players get the same nano editor as kubectl edit deployment.
        if (const char* injected = std::getenv("SMITE_EDIT_CONTENT")) {
            edited = injected;
        } else {
            Nano editor;
            bool opened = editor.open(name + "-ingress.yaml", original,
                [&edited](const std::string& content) { edited = content; });
            if (!opened) {
                return "Error: failed to open editor\n";
            }
        }

        std::string error;
        if (!apply_ingress_yaml(*it, edited, error)) return error;
        return "ingress.networking.k8s.io/" + name + " edited\n";
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_EDIT_INGRESS_H
