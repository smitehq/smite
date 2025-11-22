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
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <regex>
#include <random>
#include <optional>

// Forward declare SimulationConfig to avoid circular dependency
// The actual include is done via forward declaration pattern
namespace k8s_simulation {
    struct SimulationConfig;
    struct Trigger;
    struct Condition;
    struct Action;
    struct SimulationRule;
}

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
    std::unordered_map<std::string, std::string> labels;  // pod labels for selector matching
};

struct Node {
    std::string name;
    std::string ip;
    std::vector<Pod> pods;
    bool cordoned = false;  // Track if node is cordoned (unschedulable)
};

struct Secret {
    std::string name;
    std::string ns = "default";
    std::string type = "Opaque";
    std::unordered_map<std::string, std::string> data;  // key-value pairs
    std::string age = "0s";  // How long the secret has existed
};

struct ConfigMap {
    std::string name;
    std::string ns = "default";
    std::unordered_map<std::string, std::string> data;  // key-value pairs
    std::string age = "0s";
};

struct PersistentVolumeClaim {
    std::string name;
    std::string ns = "default";
    std::string status = "Pending";  // "Pending", "Bound", "Lost"
    std::string volume_name;  // Name of bound PV
    std::string storage_request;  // e.g., "1Gi", "10Gi"
    std::string storage_class;  // e.g., "standard", "fast-ssd"
    std::string age = "0s";
};

struct Deployment {
    std::string name;
    std::string ns = "default";
    int replicas = 1;
    int ready_replicas = 0;
    int available_replicas = 0;
    std::string image;
    std::string age = "0s";
    std::unordered_map<std::string, std::string> labels;
    int revision = 1;  // For rollout history
    std::string memory_limit;  // e.g., "256Mi", "512Mi", "1Gi"
    std::string cpu_limit;     // e.g., "100m", "500m", "1"
    int liveness_initial_delay = 0;  // initialDelaySeconds for liveness probe
    int liveness_timeout = 0;        // timeoutSeconds for liveness probe
    std::string affinity_type;       // "none", "required", "preferred"
};

struct StatefulSet {
    std::string name;
    std::string ns = "default";
    int replicas = 1;
    int ready_replicas = 0;
    std::string image;
    std::string age = "0s";
};

struct Service {
    std::string name;
    std::string ns = "default";
    std::string type = "ClusterIP";  // ClusterIP, NodePort, LoadBalancer
    std::string cluster_ip;
    std::string external_ip = "<none>";
    std::vector<std::string> ports;  // "80:8080/TCP"
    std::unordered_map<std::string, std::string> selector;
    std::string age = "0s";
};

struct IngressRule {
    std::string host;
    std::string path;
    std::string service_name;
    int service_port;
};

struct Ingress {
    std::string name;
    std::string ns = "default";
    std::vector<IngressRule> rules;
    std::string age = "0s";
};

struct NetworkPolicy {
    std::string name;
    std::string ns = "default";
    std::string age = "0s";
};

struct Cluster {
    std::string name;
    std::vector<Node> nodes;
    std::vector<Secret> secrets;
    std::vector<ConfigMap> configmaps;
    std::vector<Deployment> deployments;
    std::vector<Service> services;
};

class KubernetesModule : public SmiteModule {
public:
    KubernetesModule();  // Defined in .cpp where SimulationConfig is complete
    ~KubernetesModule() override;  // Need custom destructor to stop simulation thread

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
    std::string get_active_quest_id() const override { return active_quest_id; }
    bool is_quest_just_completed() const override { return quest_just_completed; }
    void clear_quest_completion_flag() override { quest_just_completed = false; }

private:
    std::string path;
    std::vector<Pod> pods;
    std::vector<Node> nodes;
    std::vector<Secret> secrets;
    std::vector<ConfigMap> configmaps;
    std::vector<PersistentVolumeClaim> pvcs;
    std::vector<Deployment> deployments;
    std::vector<StatefulSet> statefulsets;
    std::vector<Service> services;
    std::vector<Ingress> ingresses;
    std::vector<NetworkPolicy> network_policies;
    YAML::Node default_state_yaml;
    std::string active_quest_id;
    std::unordered_map<std::string, YAML::Node> quest_data;  // quest_id -> quest YAML
    bool quest_completed = false;  // Track if current quest has been completed
    bool quest_just_completed = false;  // Flag for engine to detect completion
    std::string last_command_executed;  // Track last command for quest checking

    //--------------------------------------
    // Simulation system
    //--------------------------------------
    std::unique_ptr<k8s_simulation::SimulationConfig> simulation_config_;
    std::thread simulation_thread_;
    std::atomic<bool> simulation_running_{false};
    mutable std::mutex state_mutex_;  // Protects all cluster state (nodes, pods, deployments, etc.)
    std::chrono::steady_clock::time_point quest_start_time_;

    void start_simulation();
    void stop_simulation();
    void simulation_loop();
    float get_elapsed_time() const;

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
    const std::vector<ConfigMap>& get_configmaps() const { return configmaps; }
    void add_configmap(const ConfigMap& cm) { configmaps.push_back(cm); }
    const std::vector<PersistentVolumeClaim>& get_pvcs() const { return pvcs; }
    std::vector<PersistentVolumeClaim>& get_pvcs_mutable() { return pvcs; }
    const std::vector<Deployment>& get_deployments() const { return deployments; }
    std::vector<Deployment>& get_deployments_mutable() { return deployments; }
    const std::vector<Service>& get_services() const { return services; }
    void add_service(const Service& svc) { services.push_back(svc); }
    std::string check_secret_trigger(const std::string& secret_name);

    // Helper methods (public for command headers)
    auto find_pod(const std::string& pod_name) -> decltype(pods.begin());
    auto find_node(const std::string& node_name) -> decltype(nodes.begin());
    auto find_secret(const std::string& secret_name) -> std::vector<Secret>::iterator;
    auto pod_end() -> decltype(pods.end());
    auto secret_end() -> decltype(secrets.end());
    int random_usage(int max);

    // Simulation helpers (public for simulation.cpp)
    bool is_quest_completed() const { return quest_completed; }
    std::mutex& get_state_mutex() { return state_mutex_; }
};

#endif // MODULES_KUBERNETES_MODULE_H
