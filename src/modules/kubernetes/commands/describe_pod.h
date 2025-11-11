#ifndef MODULES_KUBERNETES_COMMANDS_DESCRIBE_POD_H
#define MODULES_KUBERNETES_COMMANDS_DESCRIBE_POD_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>

namespace kubectl_commands {

inline auto cmd_describe_pod(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        if (args.empty()) return "Error: pod name required\n";

        auto it = module->find_pod(args[0]);
        if (it == module->pod_end()) {
            std::ostringstream out;
            out << "Error from server (NotFound): pod \"" << args[0] << "\" not found\n";
            // fuzzy match suggestions
            for (const auto& node : module->get_nodes()) {
                for (const auto& pp : node.pods) {
                    if (pp.name.rfind(args[0], 0) == 0) {
                        out << "Did you mean \"" << pp.name << "\"?\n";
                    }
                }
            }
            return out.str();
        }

        const Pod& pod = *it;
        std::ostringstream out;

        // Basic info
        out << "Name:       " << pod.name << "\n"
            << "Namespace:  " << pod.ns << "\n"
            << "Status:     " << pod.status << "\n"
            << "Restarts:   " << pod.restarts << "\n"
            << "IP:         " << (pod.ip.empty() ? "10.244.0.0" : pod.ip) << "\n"
            << "Image:      " << (pod.image.empty() ? "placeholder-image:v1" : pod.image) << "\n"
            << "Ready:      " << (pod.status == "Running" ? "True" : "False") << "\n";

        // Container state
        out << "Containers:\n"
            << "  " << pod.name << "-container:\n"
            << "    State: " << pod.container_state << "\n";

        if (pod.container_state == "Terminated") {
            out << "    Last State: Terminated\n"
                << "      Reason: " << pod.last_state.reason << "\n"
                << "      Exit Code: " << pod.last_state.exit_code << "\n"
                << "      Started: " << pod.last_state.started << "\n"
                << "      Finished: " << pod.last_state.finished << "\n";
        }

        // Events
        out << "Events:\n";
        for (const auto& e : pod.events) {
            out << "  "
                << std::left << std::setw(8) << e.type    // type column, width 8
                << std::setw(10) << e.reason             // reason column, width 10
                << std::setw(20) << e.timestamp          // timestamp, width 20
                << "kubelet  "
                << e.message << "\n";
        }

        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_DESCRIBE_POD_H
