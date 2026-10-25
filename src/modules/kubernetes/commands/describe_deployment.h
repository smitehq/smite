#ifndef MODULES_KUBERNETES_COMMANDS_DESCRIBE_DEPLOYMENT_H
#define MODULES_KUBERNETES_COMMANDS_DESCRIBE_DEPLOYMENT_H

#include "modules/kubernetes/module.h"
#include <algorithm>
#include <sstream>

namespace kubectl_commands {

inline auto cmd_describe_deployment(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) {
            return "Error: deployment name required\n"
                   "Usage: kubectl describe deployment <name>\n";
        }

        const auto& deployments = module->get_deployments();
        auto it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == args[0]; });

        if (it == deployments.end()) {
            return "Error from server (NotFound): deployments.apps \"" + args[0] + "\" not found\n";
        }

        std::ostringstream out;
        out << "Name:               " << it->name << "\n"
            << "Namespace:          " << it->ns << "\n"
            << "Replicas:           " << it->ready_replicas << "/" << it->replicas << "\n"
            << "Available:          " << it->available_replicas << "\n"
            << "Image:              " << (it->image.empty() ? "<none>" : it->image) << "\n"
            << "Revision:           " << it->revision << "\n"
            << "Age:                " << it->age << "\n";
        if (!it->memory_limit.empty()) {
            out << "Memory Limit:       " << it->memory_limit << "\n";
        }
        if (!it->cpu_limit.empty()) {
            out << "CPU Limit:          " << it->cpu_limit << "\n";
        }
        if (!it->affinity_type.empty()) {
            out << "Affinity:           " << it->affinity_type << "\n";
        }
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_DESCRIBE_DEPLOYMENT_H
