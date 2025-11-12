#ifndef MODULES_KUBERNETES_COMMANDS_EDIT_DEPLOYMENT_H
#define MODULES_KUBERNETES_COMMANDS_EDIT_DEPLOYMENT_H

#include "modules/kubernetes/module.h"
#include "shell/nano.h"
#include <sstream>
#include <yaml-cpp/yaml.h>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_edit_deployment(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) {
            return "Error: deployment name required\n";
        }

        std::string deployment_name = args[0];

        // Find the deployment
        auto& deployments = module->get_deployments_mutable();
        auto dep_it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == deployment_name; });

        if (dep_it == deployments.end()) {
            return "Error from server (NotFound): deployment.apps \"" + deployment_name + "\" not found\n";
        }

        // Generate deployment YAML from current state
        std::ostringstream yaml;
        yaml << "apiVersion: apps/v1\n"
             << "kind: Deployment\n"
             << "metadata:\n"
             << "  name: " << deployment_name << "\n"
             << "spec:\n"
             << "  replicas: " << dep_it->replicas << "\n"
             << "  template:\n"
             << "    spec:\n"
             << "      containers:\n"
             << "      - name: " << deployment_name << "\n"
             << "        image: " << dep_it->image << "\n";

        // Add resources if defined
        if (!dep_it->memory_limit.empty() || !dep_it->cpu_limit.empty()) {
            yaml << "        resources:\n"
                 << "          limits:\n";
            if (!dep_it->memory_limit.empty()) {
                yaml << "            memory: " << dep_it->memory_limit << "\n";
            }
            if (!dep_it->cpu_limit.empty()) {
                yaml << "            cpu: " << dep_it->cpu_limit << "\n";
            }
        }

        // Add liveness probe if defined
        if (dep_it->liveness_initial_delay > 0) {
            yaml << "        livenessProbe:\n"
                 << "          httpGet:\n"
                 << "            path: /health\n"
                 << "            port: 8080\n"
                 << "          initialDelaySeconds: " << dep_it->liveness_initial_delay << "\n"
                 << "          timeoutSeconds: " << dep_it->liveness_timeout << "\n";
        }

        // Add affinity if defined
        if (!dep_it->affinity_type.empty() && dep_it->affinity_type != "none") {
            yaml << "      affinity:\n"
                 << "        podAffinity:\n";
            if (dep_it->affinity_type == "required") {
                yaml << "          requiredDuringSchedulingIgnoredDuringExecution:\n"
                     << "          - labelSelector:\n"
                     << "              matchLabels:\n"
                     << "                app: cache\n"
                     << "            topologyKey: kubernetes.io/hostname\n";
            } else if (dep_it->affinity_type == "preferred") {
                yaml << "          preferredDuringSchedulingIgnoredDuringExecution:\n"
                     << "          - weight: 100\n"
                     << "            podAffinityTerm:\n"
                     << "              labelSelector:\n"
                     << "                matchLabels:\n"
                     << "                  app: cache\n"
                     << "              topologyKey: kubernetes.io/hostname\n";
            }
        }

        std::string deployment_yaml = yaml.str();

        // Open nano editor
        Nano editor;
        std::string edited_yaml = deployment_yaml;
        bool editor_success = editor.open(deployment_name + "-deployment.yaml", deployment_yaml,
            [&edited_yaml](const std::string& new_content) {
                edited_yaml = new_content;
            });

        if (!editor_success) {
            return "Error: failed to open editor\n";
        }

        // Parse the edited YAML and update the deployment
        try {
            YAML::Node edited = YAML::Load(edited_yaml);

            // Update replicas
            if (edited["spec"] && edited["spec"]["replicas"]) {
                dep_it->replicas = edited["spec"]["replicas"].as<int>();
            }

            // Update resources
            if (edited["spec"] && edited["spec"]["template"] &&
                edited["spec"]["template"]["spec"] &&
                edited["spec"]["template"]["spec"]["containers"] &&
                edited["spec"]["template"]["spec"]["containers"][0]) {

                auto container = edited["spec"]["template"]["spec"]["containers"][0];

                if (container["resources"] && container["resources"]["limits"]) {
                    auto limits = container["resources"]["limits"];
                    if (limits["memory"]) {
                        dep_it->memory_limit = limits["memory"].as<std::string>();
                    }
                    if (limits["cpu"]) {
                        dep_it->cpu_limit = limits["cpu"].as<std::string>();
                    }
                }

                // Update liveness probe
                if (container["livenessProbe"]) {
                    auto probe = container["livenessProbe"];
                    if (probe["initialDelaySeconds"]) {
                        dep_it->liveness_initial_delay = probe["initialDelaySeconds"].as<int>();
                    }
                    if (probe["timeoutSeconds"]) {
                        dep_it->liveness_timeout = probe["timeoutSeconds"].as<int>();
                    }
                }
            }

            // Update affinity
            if (edited["spec"] && edited["spec"]["template"] &&
                edited["spec"]["template"]["spec"]) {

                if (edited["spec"]["template"]["spec"]["affinity"]) {
                    auto affinity = edited["spec"]["template"]["spec"]["affinity"];

                    if (affinity["podAffinity"]) {
                        auto podAffinity = affinity["podAffinity"];
                        if (podAffinity["requiredDuringSchedulingIgnoredDuringExecution"]) {
                            dep_it->affinity_type = "required";
                        } else if (podAffinity["preferredDuringSchedulingIgnoredDuringExecution"]) {
                            dep_it->affinity_type = "preferred";
                        } else {
                            dep_it->affinity_type = "none";
                        }
                    } else {
                        dep_it->affinity_type = "none";
                    }
                } else {
                    // No affinity section means no affinity
                    dep_it->affinity_type = "none";
                }
            }

        } catch (const YAML::Exception& e) {
            return "Error parsing YAML: " + std::string(e.what()) + "\n";
        }

        return "deployment.apps/" + deployment_name + " edited\n";
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_EDIT_DEPLOYMENT_H
