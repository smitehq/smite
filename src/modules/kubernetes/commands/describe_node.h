#ifndef MODULES_KUBERNETES_COMMANDS_DESCRIBE_NODE_H
#define MODULES_KUBERNETES_COMMANDS_DESCRIBE_NODE_H

#include "modules/kubernetes/module.h"
#include <algorithm>
#include <sstream>
#include <vector>

namespace kubectl_commands {

inline std::string describe_one_node(const Node& node) {
    std::ostringstream out;
    const std::string cpu = node.cpu_capacity.empty() ? "4" : node.cpu_capacity;
    const std::string memory = node.memory_capacity.empty() ? "8Gi" : node.memory_capacity;

    out << "Name: " << node.name << "\n"
        << "IP: " << node.ip << "\n"
        << "Unschedulable: " << (node.cordoned ? "true" : "false") << "\n"
        << "Capacity:\n"
        << "  cpu: " << cpu << "\n"
        << "  memory: " << memory << "\n";

    out << "Labels: ";
    if (node.labels.empty()) {
        out << "<none>\n";
    } else {
        std::vector<std::pair<std::string, std::string>> labels(node.labels.begin(), node.labels.end());
        std::sort(labels.begin(), labels.end());
        for (size_t i = 0; i < labels.size(); ++i) {
            if (i) out << ",";
            out << labels[i].first << "=" << labels[i].second;
        }
        out << "\n";
    }

    out << "Pods:\n";
    if (node.pods.empty()) {
        out << "  <none>\n";
    } else {
        for (const auto& pod : node.pods) {
            out << "  " << pod.name << " (" << pod.status << ")\n";
        }
    }
    return out.str();
}

inline auto cmd_describe_node(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string {
        // "kubectl describe node <name>" and "kubectl describe nodes" (all nodes).
        std::string wanted;
        for (const auto& arg : args) {
            if (!arg.empty() && arg[0] != '-') {
                wanted = arg;
                break;
            }
        }

        const auto& nodes = module->get_nodes();
        if (!wanted.empty()) {
            auto it = module->find_node(wanted);
            if (it == nodes.end()) {
                return "Error from server (NotFound): node \"" + wanted + "\" not found\n";
            }
            return describe_one_node(*it);
        }

        if (nodes.empty()) return "No resources found\n";

        std::ostringstream out;
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (i) out << "\n";
            out << describe_one_node(nodes[i]);
        }
        return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_DESCRIBE_NODE_H
