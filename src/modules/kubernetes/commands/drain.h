#ifndef K8S_COMMANDS_DRAIN_H
#define K8S_COMMANDS_DRAIN_H

#include "modules/kubernetes/module.h"
#include <sstream>

namespace k8s_commands {

inline auto cmd_drain(KubernetesModule* mod) {
    return [mod](const std::vector<std::string>& args) -> std::string {
        // kubectl drain <node> [--ignore-daemonsets]
        std::string node_name;
        for (const auto& arg : args) {
            if (arg.empty() || arg[0] == '-') continue;
            node_name = arg;
            break;
        }
        if (node_name.empty()) {
            return "Error: node name required\n"
                   "Usage: kubectl drain <node> --ignore-daemonsets\n";
        }

        auto& nodes = mod->get_nodes_mutable();
        Node* source = nullptr;
        Node* destination = nullptr;
        for (auto& node : nodes) {
            if (node.name == node_name) source = &node;
        }
        if (!source) {
            return "Error: node \"" + node_name + "\" not found\n";
        }
        for (auto& node : nodes) {
            if (&node != source && !node.cordoned) {
                destination = &node;
                break;
            }
        }
        if (!destination) {
            for (auto& node : nodes) {
                if (&node != source) {
                    destination = &node;
                    break;
                }
            }
        }

        source->cordoned = true;
        std::ostringstream out;
        out << "node/" << node_name << " cordoned\n";

        if (destination && !source->pods.empty()) {
            for (const auto& pod : source->pods) {
                out << "evicting pod " << pod.name << "\n";
            }
            destination->pods.insert(destination->pods.end(), source->pods.begin(), source->pods.end());
            source->pods.clear();
        }

        out << "node/" << node_name << " drained\n";
        return out.str();
    };
}

} // namespace k8s_commands

#endif // K8S_COMMANDS_DRAIN_H
