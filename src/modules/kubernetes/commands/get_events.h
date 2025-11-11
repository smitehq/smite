#ifndef MODULES_KUBERNETES_COMMANDS_GET_EVENTS_H
#define MODULES_KUBERNETES_COMMANDS_GET_EVENTS_H

#include "../module.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>

namespace kubectl_commands {

inline auto cmd_get_events(KubernetesModule* module) {
    return [module](const std::vector<std::string>& args) -> std::string  {
    bool wide = !args.empty() && (args[0] == "wide" || (args[0] == "-o" && args.size() > 1 && args[1] == "wide"));

    struct EventRow {
        std::string timestamp;
        std::string type;
        std::string reason;
        std::string object;
        std::string node;
        std::string message;
    };

    std::vector<EventRow> all_events;

    // Gather all events
    for (const auto& node : module->get_nodes()) {
        for (const auto& pod : node.pods) {
            for (const auto& e : pod.events) {
                all_events.push_back({
                    e.timestamp,
                    e.type,
                    e.reason,
                    pod.name,
                    node.name,
                    e.message
                });
            }
        }
    }

    // Sort by timestamp descending
    std::sort(all_events.begin(), all_events.end(), [](const EventRow& a, const EventRow& b) {
        return a.timestamp > b.timestamp;
    });

    // Compute column widths dynamically
    size_t ts_width     = 9;
    size_t type_width   = 4;
    size_t reason_width = 6;
    size_t obj_width    = 6;
    size_t node_width   = 4;
    size_t msg_width    = 20; // declare here, in outer scope

    for (const auto& e : all_events) {
        ts_width     = std::max(ts_width, e.timestamp.size());
        type_width   = std::max(type_width, e.type.size());
        reason_width = std::max(reason_width, e.reason.size());
        obj_width    = std::max(obj_width, e.object.size());
        node_width   = std::max(node_width, e.node.size());
        if (wide) msg_width = std::max(msg_width, e.message.size());
    }

    std::ostringstream out;

    // Header
    out << std::left
        << std::setw(ts_width + 2) << "LAST SEEN"
        << std::setw(type_width + 2) << "TYPE"
        << std::setw(reason_width + 2) << "REASON"
        << std::setw(obj_width + 2) << "OBJECT";

    if (wide) out << std::setw(node_width + 2) << "NODE";

    out << "MESSAGE\n";

    // Rows
    for (const auto& e : all_events) {
        out << std::setw(ts_width + 2) << e.timestamp
            << std::setw(type_width + 2) << e.type
            << std::setw(reason_width + 2) << e.reason
            << std::setw(obj_width + 2) << e.object;

        if (wide) out << std::setw(node_width + 2) << e.node;

        std::string msg = e.message;
        if (!wide && msg.size() > msg_width) msg = msg.substr(0, msg_width - 3) + "...";

        out << msg << "\n";
    }

    return out.str();
    };
}

} // namespace kubectl_commands

#endif // MODULES_KUBERNETES_COMMANDS_GET_EVENTS_H
