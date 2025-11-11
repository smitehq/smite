#ifndef MODULES_KUBERNETES_COMMANDS_GET_PODS_H
#define MODULES_KUBERNETES_COMMANDS_GET_PODS_H

#include "../module.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_get_pods(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        bool wide = !args.empty() && (args[0] == "wide" || (args[0] == "-o" && args.size() > 1 && args[1] == "wide"));
        std::ostringstream out;

        size_t name_width   = 4;
        size_t ready_width  = 5;
        size_t status_width = 6;
        size_t rest_width   = 8;
        size_t node_width   = 4;
        size_t ip_width     = 2;
        size_t image_width  = 5;

        // Compute column widths
        for (const auto& node : module->get_nodes()) {
            node_width = std::max(node_width, node.name.size());
            for (const auto& p : node.pods) {
                name_width   = std::max(name_width, p.name.size());
                status_width = std::max(status_width, p.status.size());
                rest_width   = std::max(rest_width, std::to_string(p.restarts).size());
                ip_width     = std::max(ip_width, p.ip.empty() ? 10u : p.ip.size());
                image_width  = std::max(image_width, p.image.empty() ? 16u : p.image.size());
            }
        }

        // Header
        out << std::left
            << std::setw(name_width+2)   << "NAME"
            << std::setw(ready_width+2)  << "READY"
            << std::setw(status_width+2) << "STATUS"
            << std::setw(rest_width+2)   << "RESTARTS";

        if (wide) {
            out << std::setw(node_width+2) << "NODE"
                << std::setw(ip_width+2)   << "IP"
                << std::setw(image_width+2)<< "IMAGE";
        }

        out << "\n";

        // Rows
        for (const auto& node : module->get_nodes()) {
            for (const auto& p : node.pods) {
                out << std::left
                    << std::setw(name_width+2)   << p.name
                    << std::setw(ready_width+2)  << (p.status == "Running" ? "1/1" : "0/1")
                    << std::setw(status_width+2) << p.status
                    << std::setw(rest_width+2)   << p.restarts;

                if (wide) {
                    out << std::setw(node_width+2) << node.name
                        << std::setw(ip_width+2)   << (p.ip.empty() ? "10.244.0.0" : p.ip)
                        << std::setw(image_width+2)<< (p.image.empty() ? "placeholder:v1" : p.image);
                }

                out << "\n";
            }
        }

        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_GET_PODS_H
