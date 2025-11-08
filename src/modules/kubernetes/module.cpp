#include "module.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <iostream>
#include <random>

namespace fs = std::filesystem;

std::string KubernetesModule::name() const { return "kubernetes"; }

bool KubernetesModule::load_from_path(const std::string& modulePath) {
    path = modulePath;
    bool all_good = true;

    struct FileConfig {
        std::string filename;
        bool optional;
        std::function<void(const YAML::Node&)> handler;
    };

    std::vector<FileConfig> configs = {
        {"state.yaml", false, [this](const YAML::Node& node) {
            if (node["cluster"] && node["cluster"]["nodes"]) {
                for (auto n : node["cluster"]["nodes"]) {
                    Node node_struct;
                    if (n["name"]) node_struct.name = n["name"].as<std::string>();
                    if (n["ip"]) node_struct.ip = n["ip"].as<std::string>();
                    
                    if (n["pods"]) {
                        for (auto p : n["pods"]) {
                            Pod pod;
                            if (p["name"]) pod.name = p["name"].as<std::string>();
                            if (p["status"]) pod.status = p["status"].as<std::string>();
                            if (p["restarts"]) pod.restarts = p["restarts"].as<int>();
                            if (p["image"]) pod.image = p["image"].as<std::string>();
                            if (p["container_state"]) pod.container_state = p["container_state"].as<std::string>();

                            if (p["last_state"]) {
                                const auto& ls = p["last_state"];
                                if (ls["reason"]) pod.last_state.reason = ls["reason"].as<std::string>();
                                if (ls["exit_code"]) pod.last_state.exit_code = ls["exit_code"].as<int>();
                                if (ls["started"]) pod.last_state.started = ls["started"].as<std::string>();
                                if (ls["finished"]) pod.last_state.finished = ls["finished"].as<std::string>();
                            }

                            if (p["events"]) {
                                for (auto e : p["events"]) {
                                    PodEvent evt;
                                    if (e["type"]) evt.type = e["type"].as<std::string>();
                                    if (e["reason"]) evt.reason = e["reason"].as<std::string>();
                                    if (e["message"]) evt.message = e["message"].as<std::string>();
                                    if (e["timestamp"]) evt.timestamp = e["timestamp"].as<std::string>();
                                    pod.events.push_back(evt);
                                }
                            }

                            if (p["logs"]) {
                                for (auto l : p["logs"]) {
                                    PodLog log;
                                    if (l["timestamp"]) log.timestamp = l["timestamp"].as<std::string>();
                                    if (l["message"])   log.message   = l["message"].as<std::string>();
                                    pod.logs.push_back(log);
                                }
                            }

                            node_struct.pods.push_back(pod);
                        }
                    }

                    nodes.push_back(node_struct); // <-- top-level vector now
                }
            }

        }},
        {"quests.yaml", true, [this](const YAML::Node& node) {
            quests = node;
            // std::cout << "Loaded quests.yaml with " << quests.size() << " top-level keys\n";
        }}
    };

    for (const auto& config : configs) {
        std::string file_path = (fs::path(modulePath) / config.filename).string();
        if (!fs::exists(file_path)) {
            std::cout << "Warning: " << config.filename << " missing at " << file_path << "\n";
            if (!config.optional) all_good = false;
            continue;
        }
        try {
            YAML::Node node = YAML::LoadFile(file_path);
            config.handler(node);
        } catch (const std::exception& ex) {
            std::cout << config.filename << " error: " << ex.what() << "\n";
            if (!config.optional) all_good = false;
        }
    }

    register_builtin_commands();

    // std::cout << "Module load complete (all good: " << (all_good ? "true" : "false") << ")\n";
    return all_good;
}

void KubernetesModule::register_command(const std::string& name, CommandHandler handler) {
    command_registry[name] = std::move(handler);
}

void KubernetesModule::register_builtin_commands() {
    register_command("kubectl version", [this](const auto& args) -> std::string {
        std::ostringstream out;
        out << "Client Version: v1.28.0\n";
        out << "Server Version: v1.28.0 (simulated)\n";
        return out.str();
    });

    register_command("kubectl get pods", [this](const auto& args) -> std::string {
        bool wide = !args.empty() && (args[0] == "wide" || (args[0] == "-o" && args[1] == "wide"));
        std::ostringstream out;

        size_t name_width   = 4;
        size_t ready_width  = 5;
        size_t status_width = 6;
        size_t rest_width   = 8;
        size_t node_width   = 4;
        size_t ip_width     = 2;
        size_t image_width  = 5;

        // Compute column widths
        for (const auto& node : nodes) {
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
        for (const auto& node : nodes) {
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
    });

    register_command("kubectl logs", [this](const auto& args) -> std::string {
        if (args.empty()) return "Error: pod name required\n";

        auto it = find_pod(args[0]);
        if (it == decltype(nodes[0].pods.begin()){ }) {
            std::ostringstream out;
            out << "Error from server (NotFound): pod \"" << args[0] << "\" not found\n";
            for (const auto& node : nodes) 
                for (const auto& p : node.pods)
                    if (p.name.rfind(args[0], 0) == 0)
                        out << "Did you mean \"" << p.name << "\"?\n";
            return out.str();
        }

        std::ostringstream out;
        const Pod& pod = *it;
        for (const auto& log : pod.logs) {
            out << log.timestamp << " " << pod.name << ": " << log.message << "\n";
        }
        return out.str();
    });

    register_command("kubectl describe pod", [this](const auto& args) -> std::string {
        if (args.empty()) return "Error: pod name required\n";

        auto it = find_pod(args[0]);
        if (it == pods.end()) {
            std::ostringstream out;
            out << "Error from server (NotFound): pod \"" << args[0] << "\" not found\n";
            // fuzzy match suggestions
            for (const auto& pp : pods) {
                if (pp.name.rfind(args[0], 0) == 0) {
                    out << "Did you mean \"" << pp.name << "\"?\n";
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
    });

    register_command("kubectl edit deployment", [this](const auto&) -> std::string {
        auto it = std::find_if(pods.begin(), pods.end(), [](const Pod& pp) { return pp.name == "backend"; });
        std::ostringstream out;
        if (it != pods.end()) {
            it->status = "Running";
            it->restarts = 0;
            it->logs.push_back(PodLog{
                "2025-11-08T13:00:00", // some timestamp
                "Info: service connected to database successfully"
            });
            out << "Deployment updated successfully. Service restored!\n";
        } else out << "No backend pod found to edit\n";
        return out.str();
    });

    register_command("kubectl get events", [this](const auto& args) -> std::string {
        bool wide = !args.empty() && (args[0] == "wide" || (args[0] == "-o" && args[1] == "wide"));

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
        for (const auto& node : nodes) {
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
    });

    register_command("kubectl get nodes", [this](const auto&) -> std::string {
        std::ostringstream out;
        size_t name_width = 4, status_width = 7;

        out << std::left << std::setw(10) << "NAME" << std::setw(12) << "STATUS" << "ROLES\n";
        for (const auto& n : nodes) {
            std::string status = "Ready"; // assume all nodes ready
            std::string roles = "worker"; // default role
            out << std::setw(10) << n.name << std::setw(12) << status << roles << "\n";
        }
        return out.str();
    });

    register_command("kubectl describe node", [this](const auto& args) -> std::string {
        if (args.empty()) return "Error: node name required\n";
        auto it = find_node(args[0]);
        if (it == nodes.end()) return "Error from server (NotFound): node not found\n";

        std::ostringstream out;
        out << "Name: " << it->name << "\nIP: " << it->ip << "\n"
            << "Pods:\n";

        for (const auto& p : it->pods)
            out << "  " << p.name << " (" << p.status << ")\n";

        return out.str();
    });

    // kubectl top nodes
    register_command("kubectl top nodes", [this](const auto& args) -> std::string {
        std::ostringstream out;
        size_t name_width = 4; // NAME column

        for (const auto& n : nodes)
            name_width = std::max(name_width, n.name.size());

        out << std::left
            << std::setw(name_width + 2) << "NAME"
            << std::setw(8) << "CPU(%)"
            << "MEM(%)\n";

        for (const auto& n : nodes) {
            int cpu = random_usage(80);  // simulate CPU usage 1-80%
            int mem = random_usage(70);  // simulate Memory usage 1-70%
            out << std::setw(name_width + 2) << n.name
                << std::setw(8) << cpu
                << mem << "\n";
        }
        return out.str();
    });

    // kubectl top pods
    register_command("kubectl top pods", [this](const auto& args) -> std::string {
        std::ostringstream out;
        size_t name_width = 4;

        for (const auto& node : nodes)
            for (const auto& p : node.pods)
                name_width = std::max(name_width, p.name.size());

        out << std::left
            << std::setw(name_width + 2) << "NAME"
            << std::setw(8) << "CPU(%)"
            << "MEM(%)\n";

        for (const auto& node : nodes) {
            for (const auto& p : node.pods) {
                int cpu = random_usage(50);  // simulate pod CPU usage 1-50%
                int mem = random_usage(50);  // simulate pod MEM usage 1-50%
                out << std::setw(name_width + 2) << p.name
                    << std::setw(8) << cpu
                    << mem << "\n";
            }
        }
        return out.str();
    });
}

auto KubernetesModule::find_pod(const std::string& pod_name) -> decltype(pods.begin()) {
    for (auto& node : nodes) {
        auto it = std::find_if(node.pods.begin(), node.pods.end(), [&](const Pod& pp){ 
            return pp.name == pod_name;
        });
        if (it != node.pods.end()) return it;
    }
    return decltype(nodes[0].pods.begin()){}; // not found
}

auto KubernetesModule::find_node(const std::string& node_name) -> decltype(nodes.begin()) {
    return std::find_if(nodes.begin(), nodes.end(), [&](const Node& n){ return n.name == node_name; });
}

bool KubernetesModule::supports_command(const std::string& cmdPrefix) const {
    return command_registry.find(cmdPrefix) != command_registry.end();
}

std::string KubernetesModule::run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) {
    auto it = command_registry.find(cmdPrefix);
    if (it != command_registry.end()) return it->second(args);
    return "Command supported but not implemented in module\n";
}

bool KubernetesModule::evaluate_condition(const YAML::Node& conditionSpec) {
    if (!conditionSpec || !conditionSpec["type"]) return false;
    std::string t = conditionSpec["type"].as<std::string>();
    if (t == "pod_status") {
        std::string name = conditionSpec["pod"].as<std::string>();
        std::string expect = conditionSpec["status"].as<std::string>();
        auto it = find_pod(name);
        if (it == pods.end()) return false;
        return it->status == expect;
    }
    return false;
}

std::vector<std::string> KubernetesModule::registered_prefixes() const {
    std::vector<std::string> out;
    for (const auto& kv : command_registry) out.push_back(kv.first);
    return out;
}

// Helper to generate random percentage/usage
int KubernetesModule::random_usage(int max) {
    static std::mt19937 rng(std::random_device{}());
    std::uniform_int_distribution<int> dist(1, max);
    return dist(rng);
};

// Factory
std::shared_ptr<SmiteModule> create_module_kubernetes() {
    return std::make_shared<KubernetesModule>();
}
