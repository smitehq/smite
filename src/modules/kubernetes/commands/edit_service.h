#ifndef K8S_COMMANDS_EDIT_SERVICE_H
#define K8S_COMMANDS_EDIT_SERVICE_H

#include "modules/kubernetes/module.h"
#include "shell/nano.h"
#include <sstream>
#include <yaml-cpp/yaml.h>

namespace k8s_commands {

inline auto cmd_edit_service(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string {
        // kubectl edit service <name>

        if (args.empty()) {
            return "Error: service name required\n";
        }

        std::string service_name = args[0];

        // Find the service
        auto& services = const_cast<std::vector<Service>&>(module->get_services());
        auto svc_it = std::find_if(services.begin(), services.end(),
            [&](const Service& s) { return s.name == service_name; });

        if (svc_it == services.end()) {
            return "Error from server (NotFound): services \"" + service_name + "\" not found\n";
        }

        Service& svc = *svc_it;

        // Generate YAML from current service state
        std::ostringstream yaml_content;
        yaml_content << "apiVersion: v1\n";
        yaml_content << "kind: Service\n";
        yaml_content << "metadata:\n";
        yaml_content << "  name: " << svc.name << "\n";
        yaml_content << "  namespace: " << svc.ns << "\n";
        yaml_content << "spec:\n";
        yaml_content << "  type: " << svc.type << "\n";
        yaml_content << "  selector:\n";
        for (const auto& [key, val] : svc.selector) {
            yaml_content << "    " << key << ": " << val << "\n";
        }
        yaml_content << "  ports:\n";
        for (const auto& port : svc.ports) {
            yaml_content << "    - port: 80\n";
            yaml_content << "      targetPort: 8080\n";
            yaml_content << "      protocol: TCP\n";
        }

        // Open editor
        std::string service_yaml = yaml_content.str();
        Nano editor;
        std::string edited_yaml = service_yaml;
        bool editor_success = editor.open(service_name + "-service.yaml", service_yaml,
            [&edited_yaml](const std::string& new_content) {
                edited_yaml = new_content;
            });

        if (!editor_success) {
            return "Error: failed to open editor\n";
        }

        if (edited_yaml.empty() || edited_yaml == service_yaml) {
            return "Edit cancelled, no changes made\n";
        }

        // Parse edited YAML and update service
        try {
            YAML::Node edited_yaml_node = YAML::Load(edited_yaml);

            // Update selector
            if (edited_yaml_node["spec"] && edited_yaml_node["spec"]["selector"]) {
                svc.selector.clear();
                for (const auto& kv : edited_yaml_node["spec"]["selector"]) {
                    svc.selector[kv.first.as<std::string>()] = kv.second.as<std::string>();
                }
            }

            // Update type
            if (edited_yaml_node["spec"] && edited_yaml_node["spec"]["type"]) {
                svc.type = edited_yaml_node["spec"]["type"].as<std::string>();
            }

            return "service/" + service_name + " edited\n";

        } catch (const YAML::Exception& e) {
            return "Error parsing edited YAML: " + std::string(e.what()) + "\n";
        }
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_EDIT_SERVICE_H
