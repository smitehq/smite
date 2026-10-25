#ifndef MODULES_KUBERNETES_COMMANDS_GET_PODS_H
#define MODULES_KUBERNETES_COMMANDS_GET_PODS_H

#include "modules/kubernetes/module.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <vector>

namespace kubectl_commands {

inline auto cmd_get_pods(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
        bool wide = false;
        bool show_labels = false;
        for (size_t i = 0; i < args.size(); ++i) {
            if (args[i] == "--show-labels") {
                show_labels = true;
            } else if (args[i] == "wide" || args[i] == "-o=wide") {
                wide = true;
            } else if (args[i] == "-o" && i + 1 < args.size() && args[i + 1] == "wide") {
                wide = true;
                ++i;
            }
        }

        auto format_labels = [](const Pod& pod) {
            if (pod.labels.empty()) return std::string("<none>");
            std::vector<std::pair<std::string, std::string>> labels(pod.labels.begin(), pod.labels.end());
            std::sort(labels.begin(), labels.end());
            std::string text;
            for (size_t i = 0; i < labels.size(); ++i) {
                if (i) text += ",";
                text += labels[i].first + "=" + labels[i].second;
            }
            return text;
        };

        std::ostringstream out;

        size_t name_width   = 4;
        size_t ready_width  = 5;
        size_t status_width = 6;
        size_t rest_width   = 8;
        size_t node_width   = 4;
        size_t ip_width     = 2;
        size_t image_width  = 5;
        size_t label_width  = 6;

        // Compute column widths
        for (const auto& node : module->get_nodes()) {
            node_width = std::max(node_width, node.name.size());
            for (const auto& p : node.pods) {
                name_width   = std::max(name_width, p.name.size());
                status_width = std::max(status_width, p.status.size());
                rest_width   = std::max(rest_width, std::to_string(p.restarts).size());
                ip_width     = std::max(ip_width, p.ip.empty() ? 10u : p.ip.size());
                image_width  = std::max(image_width, p.image.empty() ? 16u : p.image.size());
                if (show_labels) {
                    label_width = std::max(label_width, format_labels(p).size());
                }
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
        if (show_labels) {
            out << std::setw(label_width + 2) << "LABELS";
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
                if (show_labels) {
                    out << format_labels(p);
                }

                out << "\n";
            }
        }

        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_GET_PODS_H
