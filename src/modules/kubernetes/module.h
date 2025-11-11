#ifndef MODULES_KUBERNETES_MODULE_H
#define MODULES_KUBERNETES_MODULE_H

#include "core/module_interface.h"
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>
#include <algorithm>

struct PodEvent {
    std::string type;       // e.g., "Warning" or "Normal"
    std::string reason;     // e.g., "BackOff", "Started"
    std::string message;    // descriptive message
    std::string timestamp;  // optional, ISO format
};

struct LastState {
    std::string reason;     // why container terminated
    int exit_code = 0;      // exit code from container
    std::string started;    // start time
    std::string finished;   // end time
};

struct PodLog {
    std::string timestamp;
    std::string message;
};

struct Pod {
    std::string name;
    std::string ns = "default";   // default namespace
    std::string status;            // "Running", "CrashLoopBackOff", etc.
    int restarts = 0;              // number of restarts
    std::string container_state;   // e.g., "Waiting", "Terminated", "Running"
    LastState last_state;          // info about last termination
    std::vector<PodEvent> events;  // events for describe output
    std::string ip;
    std::string image;
    std::vector<PodLog> logs;      // container logs
};

struct Node {
    std::string name;
    std::string ip;
    std::vector<Pod> pods;
};

struct Secret {
    std::string name;
    std::string ns = "default";
    std::string type = "Opaque";
    std::unordered_map<std::string, std::string> data;  // key-value pairs
    std::string age = "0s";  // How long the secret has existed
};

struct Cluster {
    std::string name;
    std::vector<Node> nodes;
    std::vector<Secret> secrets;
};

class KubernetesModule : public SmiteModule {
public:
    KubernetesModule() = default;
    ~KubernetesModule() override = default;

    //--------------------------------------
    // SmiteModule interface
    //--------------------------------------
    std::string name() const override;
    bool load_from_path(const std::string& modulePath) override;
    bool supports_command(const std::string& cmdPrefix) const override;
    std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    bool evaluate_condition(const YAML::Node& conditionSpec) override;
    std::vector<std::string> registered_prefixes() const override;
    bool activate_quest(const std::string& quest_id) override;
    std::string check_quest_completion();  // Check if active quest is complete and return completion message

private:
    std::string path;
    std::vector<Pod> pods;
    std::vector<Node> nodes;
    std::vector<Secret> secrets;
    YAML::Node default_state_yaml;
    std::string active_quest_id;
    std::unordered_map<std::string, YAML::Node> quest_data;  // quest_id -> quest YAML
    bool quest_completed = false;  // Track if current quest has been completed

    //--------------------------------------
    // Command registry
    //--------------------------------------
    using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
    std::unordered_map<std::string, CommandHandler> command_registry;

    void register_builtin_commands();

    // Register command factory functions - automatically passes 'this' to factory
    template<typename FactoryFunc>
    void register_command(const std::string& name, FactoryFunc factory) {
        command_registry[name] = factory(this);
    }

    //--------------------------------------
    // State Loading
    //--------------------------------------
    void load_cluster_state(const YAML::Node& node);

    //--------------------------------------
    // Quest helpers
    //--------------------------------------
    YAML::Node get_yaml_field(const YAML::Node& node, const std::string& path);
    bool validate_edit(const std::string& original_yaml, const std::string& edited_yaml);
    void execute_actions(const YAML::Node& actions);

public:
    //--------------------------------------
    // Accessors for commands (public for command header files)
    //--------------------------------------
    const std::vector<Node>& get_nodes() const { return nodes; }
    std::vector<Node>& get_nodes_mutable() { return nodes; }
    const std::vector<Secret>& get_secrets() const { return secrets; }
    void add_secret(const Secret& secret) { secrets.push_back(secret); }
    std::string check_secret_trigger(const std::string& secret_name);

    // Helper methods (public for command headers)
    auto find_pod(const std::string& pod_name) -> decltype(pods.begin());
    auto find_node(const std::string& node_name) -> decltype(nodes.begin());
    auto find_secret(const std::string& secret_name) -> std::vector<Secret>::iterator;
    auto pod_end() -> decltype(pods.end());
    auto secret_end() -> decltype(secrets.end());
    int random_usage(int max);
};

#endif // MODULES_KUBERNETES_MODULE_H
