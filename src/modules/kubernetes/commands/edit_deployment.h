#ifndef MODULES_KUBERNETES_COMMANDS_EDIT_DEPLOYMENT_H
#define MODULES_KUBERNETES_COMMANDS_EDIT_DEPLOYMENT_H

#include "modules/kubernetes/module.h"
#include "shell/nano.h"
#include <sstream>

namespace kubectl_commands {

inline auto cmd_edit_deployment(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) {
            return "Error: deployment name required\n";
        }

        std::string deployment_name = args[0];

        // Find the pod with matching name
        auto it = module->find_pod(deployment_name);
        if (it == decltype(module->get_nodes()[0].pods.begin()){}) {
            return "Error from server (NotFound): deployment \"" + deployment_name + "\" not found\n";
        }

        // Get the deployment YAML template from the active quest (if any)
        std::string deployment_yaml;
        // Note: This would need access to quest_data, which we might need to expose
        // For now, fallback to generating basic YAML

        // Generate a basic deployment YAML
        std::ostringstream yaml;
        yaml << "apiVersion: apps/v1\n"
            << "kind: Deployment\n"
            << "metadata:\n"
            << "  name: " << deployment_name << "\n"
            << "spec:\n"
            << "  replicas: 1\n"
            << "  template:\n"
            << "    spec:\n"
            << "      containers:\n"
            << "      - name: " << deployment_name << "-container\n"
            << "        image: " << it->image << "\n";
        deployment_yaml = yaml.str();

        // Open nano editor
        Nano editor;
        std::string edited_yaml = deployment_yaml;  // Store for validation
        bool editor_success = editor.open(deployment_name + "-deployment.yaml", deployment_yaml,
            [&edited_yaml](const std::string& new_content) {
                edited_yaml = new_content;
            });

        if (!editor_success) {
            return "Error: failed to open editor\n";
        }

        // Note: validate_edit would need to be exposed or moved
        return "";
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_EDIT_DEPLOYMENT_H
