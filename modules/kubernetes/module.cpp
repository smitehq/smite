// modules/kubernetes/module.cpp
#include "core/module_interface.h"  // Adjust path
#include <yaml-cpp/yaml.h>
#include <memory>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <algorithm>  // for find_if
#include <functional>  // For std::function in FileConfig

namespace fs = std::filesystem;
using YAML::Node;

/* Internal emulated state for this module */
struct Pod {
    std::string name;
    std::string ns = "default";
    std::string status;
    int restarts = 0;
    std::vector<std::string> logs;
};

class KubernetesModule : public SmiteModule {
public:
    KubernetesModule() = default;
    ~KubernetesModule() override = default;

    std::string name() const override { return "kubernetes"; }

    bool load_from_path(const std::string& modulePath) override {
        path = modulePath;
        bool all_good = true;

        // Config for files: {filename, is_optional, post-load handler}
        struct FileConfig {
            std::string filename;
            bool optional;
            std::function<void(const Node&)> handler;
        };

        std::vector<FileConfig> configs = {
            {"state.yaml", false, [this](const Node& node) {
                if (node["cluster"] && node["cluster"]["pods"]) {
                    for (auto p : node["cluster"]["pods"]) {
                        Pod pod;
                        pod.name = p["name"].as<std::string>();
                        if (p["status"]) pod.status = p["status"].as<std::string>();
                        if (p["restarts"]) pod.restarts = p["restarts"].as<int>();
                        if (p["logs"]) for (auto l : p["logs"]) pod.logs.push_back(l.as<std::string>());
                        pods.push_back(pod);
                    }
                    std::cout << "Loaded " << pods.size() << " pods from state.yaml\n";
                }
            }},
            {"commands.yaml", false, [this](const Node& node) {
                if (node["commands"]) {
                    for (auto c : node["commands"]) {
                        registered.push_back(c.as<std::string>());
                    }
                    std::cout << "Loaded " << registered.size() << " commands from commands.yaml\n";
                }
            }},
            {"quests.yaml", true, [this](const Node& node) {
                quests = node;
                std::cout << "Loaded quests.yaml with " << quests.size() << " top-level keys\n";
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
                Node node = YAML::LoadFile(file_path);
                config.handler(node);  // Parse/assign
            } catch (const std::exception& ex) {
                std::cout << config.filename << " error: " << ex.what() << "\n";
                if (!config.optional) all_good = false;
            }
        }

        std::cout << "Module load complete (all good: " << (all_good ? "true" : "false") << ")\n";
        return all_good;
    }

    bool supports_command(const std::string& cmdPrefix) const override {
        // simple: check if cmdPrefix matches any registered prefix
        for (auto &p : registered) if (p == cmdPrefix) return true;
        return false;
    }

    std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override {
        // Helper: Find pod by name (returns iterator or end())
        auto find_pod = [&](const std::string& pod_name) -> decltype(pods.begin()) {
            return std::find_if(pods.begin(), pods.end(), [&](const Pod& pp) { return pp.name == pod_name; });
        };

        // Command handlers: prefix -> lambda (args, captures this/out)
        static const std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> handlers = {
            {"kubectl get pods", [this](const auto& /*args*/) -> std::string {
                std::ostringstream out;
                out << "NAME\t\tREADY\tSTATUS\t\tRESTARTS\n";
                for (const auto& p : pods) out << p.name << "\t0/1\t" << p.status << "\t" << p.restarts << "\n";
                return out.str();
            }},
            {"kubectl logs", [&](const auto& args) -> std::string {
                if (args.empty()) return "Error: pod name required\n";
                auto it = find_pod(args[0]);
                if (it == pods.end()) {
                    std::ostringstream out;
                    out << "Error from server (NotFound): pod \"" << args[0] << "\" not found\n";
                    for (const auto& pp : pods) if (pp.name.rfind(args[0], 0) == 0) out << "Did you mean \"" << pp.name << "\"?\n";
                    return out.str();
                }
                std::ostringstream out;
                for (const auto& l : it->logs) out << l << "\n";
                return out.str();
            }},
            {"kubectl describe pod", [&](const auto& args) -> std::string {
                if (args.empty()) return "Error: pod name required\n";
                auto it = find_pod(args[0]);
                if (it == pods.end()) return "Error from server (NotFound): pod not found\n";
                std::ostringstream out;
                out << "Name: " << it->name << "\nNamespace: " << it->ns << "\nStatus: " << it->status << "\nRestarts: " << it->restarts << "\n";
                if (it->status == "CrashLoopBackOff") out << "Events: Failed to pull secret 'db-secret'\n";
                return out.str();
            }},
            {"kubectl edit deployment api-service", [this](const auto& /*args*/) -> std::string {
                auto it = std::find_if(pods.begin(), pods.end(), [](const Pod& pp) { return pp.name == "backend"; });
                std::ostringstream out;
                if (it != pods.end()) {
                    it->status = "Running";
                    it->restarts = 0;
                    it->logs.push_back("Info: service connected to database successfully");
                    out << "Deployment updated successfully. Service restored!\n";
                } else {
                    out << "No backend pod found to edit\n";
                }
                return out.str();
            }}
        };

        auto handler_it = handlers.find(cmdPrefix);
        if (handler_it != handlers.end()) {
            return handler_it->second(args);
        }

        return "Command supported but not implemented in module\n";
    }

    bool evaluate_condition(const YAML::Node& conditionSpec) override {
        // very small condition interpreter (pod_status)
        if (!conditionSpec || !conditionSpec["type"]) return false;
        std::string t = conditionSpec["type"].as<std::string>();
        if (t == "pod_status") {
            std::string name = conditionSpec["pod"].as<std::string>();
            std::string expect = conditionSpec["status"].as<std::string>();
            auto it = std::find_if(pods.begin(), pods.end(), [&](const Pod&pp){ return pp.name==name; });
            if (it==pods.end()) return false;
            return it->status == expect;
        }
        // add more condition types as needed
        return false;
    }

    std::vector<std::string> registered_prefixes() const override { return registered; }

private:
    std::string path;
    std::vector<Pod> pods;
    std::vector<std::string> registered;
    YAML::Node quests; // raw quests; engine can iterate and call evaluate_condition on each
};

// Factory function for static linking (no extern "C")
std::shared_ptr<SmiteModule> create_module_kubernetes() {
    return std::make_shared<KubernetesModule>();
}