#include "module.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
using YAML::Node;

std::string KubernetesModule::name() const { return "kubernetes"; }

bool KubernetesModule::load_from_path(const std::string& modulePath) {
    path = modulePath;
    bool all_good = true;

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
                // std::cout << "Loaded " << pods.size() << " pods from state.yaml\n";
            }
        }},
        {"quests.yaml", true, [this](const Node& node) {
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
            Node node = YAML::LoadFile(file_path);
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
        out << "kubectl: command not found: " << args[0] << "\n";
        return out.str();
    });

    register_command("kubectl get pods", [this](const auto&) -> std::string {
        std::ostringstream out;
        out << "NAME\t\tREADY\tSTATUS\tRESTARTS\n";
        for (const auto& p : pods) out << p.name << "\t\t0/1\t" << p.status << "\t" << p.restarts << "\n";
        return out.str();
    });

    register_command("kubectl logs", [this](const auto& args) -> std::string {
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
    });

    register_command("kubectl describe pod", [this](const auto& args) -> std::string {
        if (args.empty()) return "Error: pod name required\n";
        auto it = find_pod(args[0]);
        if (it == pods.end()) return "Error from server (NotFound): pod not found\n";
        std::ostringstream out;
        out << "Name: " << it->name << "\nNamespace: " << it->ns << "\nStatus: " << it->status
            << "\nRestarts: " << it->restarts << "\n";
        if (it->status == "CrashLoopBackOff") out << "Events: Failed to pull secret 'db-secret'\n";
        return out.str();
    });

    register_command("kubectl edit deployment api-service", [this](const auto&) -> std::string {
        auto it = std::find_if(pods.begin(), pods.end(), [](const Pod& pp) { return pp.name == "backend"; });
        std::ostringstream out;
        if (it != pods.end()) {
            it->status = "Running";
            it->restarts = 0;
            it->logs.push_back("Info: service connected to database successfully");
            out << "Deployment updated successfully. Service restored!\n";
        } else out << "No backend pod found to edit\n";
        return out.str();
    });
}

auto KubernetesModule::find_pod(const std::string& pod_name) -> decltype(pods.begin()) {
    return std::find_if(pods.begin(), pods.end(), [&](const Pod& pp) { return pp.name == pod_name; });
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

// Factory
std::shared_ptr<SmiteModule> create_module_kubernetes() {
    return std::make_shared<KubernetesModule>();
}
