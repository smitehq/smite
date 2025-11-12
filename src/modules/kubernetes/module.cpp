#include "module.h"
#include "shell/nano.h"
#include "state/quest.h"
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <iostream>
#include <random>
#include <sstream>
#include <regex>

// Include kubectl command implementations
#include "commands/version.h"
#include "commands/get_pods.h"
#include "commands/logs.h"
#include "commands/describe_pod.h"
#include "commands/delete_pod.h"
#include "commands/get_secrets.h"
#include "commands/create_secret.h"
#include "commands/get_nodes.h"
#include "commands/describe_node.h"
#include "commands/top_nodes.h"
#include "commands/top_pods.h"
#include "commands/get_events.h"
#include "commands/edit_deployment.h"
#include "commands/get_deployments.h"
#include "commands/get_services.h"
#include "commands/get_configmaps.h"
#include "commands/scale.h"
#include "commands/rollout.h"
#include "commands/cordon.h"

namespace fs = std::filesystem;

std::string KubernetesModule::name() const { return "kubernetes"; }

bool KubernetesModule::load_from_path(const std::string& modulePath) {
    path = modulePath;    
    // ----------------------
    // Load Default State
    // ----------------------
    std::string default_state_file = (fs::path(modulePath) / "state" / "default.yaml").string();
    if (!fs::exists(default_state_file)) return false;

    try {
        YAML::Node default_state = YAML::LoadFile(default_state_file);
        load_cluster_state(default_state);  // Extract cluster/nodes/pods
    } catch (const std::exception& ex) {
        std::cout << "default.yaml error: " << ex.what() << "\n";
        return false;
    }

    register_builtin_commands();
    return true;
}

// ----------------------
// Load Cluster State Helper
// ----------------------
void KubernetesModule::load_cluster_state(const YAML::Node& node) {
    nodes.clear();
    secrets.clear();

    if (!node["cluster"]) return;

    // Load nodes and pods
    if (node["cluster"]["nodes"]) {
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
                            if (l["message"]) log.message = l["message"].as<std::string>();
                            pod.logs.push_back(log);
                        }
                    }

                    node_struct.pods.push_back(pod);
                }
            }

            nodes.push_back(node_struct);
        }
    }

    // Load secrets
    if (node["cluster"]["secrets"]) {
        for (auto s : node["cluster"]["secrets"]) {
            Secret secret;
            if (s["name"]) secret.name = s["name"].as<std::string>();
            if (s["type"]) secret.type = s["type"].as<std::string>();
            if (s["age"]) secret.age = s["age"].as<std::string>();
            if (s["data"]) {
                for (auto kv : s["data"]) {
                    secret.data[kv.first.as<std::string>()] = kv.second.as<std::string>();
                }
            }
            secrets.push_back(secret);
        }
    }

    // Load ingresses
    if (node["cluster"]["ingresses"]) {
        for (auto ing : node["cluster"]["ingresses"]) {
            Ingress ingress;
            if (ing["name"]) ingress.name = ing["name"].as<std::string>();
            if (ing["age"]) ingress.age = ing["age"].as<std::string>();

            if (ing["rules"]) {
                for (auto r : ing["rules"]) {
                    IngressRule rule;
                    if (r["host"]) rule.host = r["host"].as<std::string>();
                    if (r["path"]) rule.path = r["path"].as<std::string>();
                    if (r["service_name"]) rule.service_name = r["service_name"].as<std::string>();
                    if (r["service_port"]) rule.service_port = r["service_port"].as<int>();
                    ingress.rules.push_back(rule);
                }
            }
            ingresses.push_back(ingress);
        }
    }
}

// ----------------------
// Activate Quest
// ----------------------
bool KubernetesModule::activate_quest(const std::string& quest_id) {
    // Load quest definition
    fs::path quest_def_path = fs::path(path) / "quests" / (quest_id + ".yaml");
    if (fs::exists(quest_def_path)) {
        try {
            YAML::Node quest = YAML::LoadFile(quest_def_path.string());
            quest_data[quest_id] = quest;
        } catch (const std::exception& e) {
            std::cout << "Failed to load quest definition: " << e.what() << "\n";
            return false;
        }
    } else {
        std::cout << "Quest definition not found: " << quest_id << "\n";
        return false;
    }

    // Load quest-specific state file
    fs::path quest_state_path = fs::path(path) / "state" / (quest_id + ".yaml");
    if (fs::exists(quest_state_path)) {
        try {
            YAML::Node quest_state = YAML::LoadFile(quest_state_path.string());
            load_cluster_state(quest_state);
            std::cout << "Loaded quest-specific state for " << quest_id << "\n";
        } catch (const std::exception& e) {
            std::cout << "Failed to load quest state: " << e.what() << "\n";
            return false;
        }
    } else {
        // Fallback: reset to default state
        std::cout << "No quest-specific state found, using default.\n";
        load_cluster_state(default_state_yaml);
    }

    active_quest_id = quest_id;
    quest_completed = false;  // Reset completion flag
    return true;
}

void KubernetesModule::register_builtin_commands() {
    using namespace kubectl_commands;
    using namespace k8s_commands;

    // Template register_command automatically passes 'this' to command factories
    register_command("kubectl version", cmd_version);
    register_command("kubectl get pods", cmd_get_pods);
    register_command("kubectl get deployments", cmd_get_deployments);
    register_command("kubectl get deploy", cmd_get_deployments);  // alias
    register_command("kubectl get services", cmd_get_services);
    register_command("kubectl get svc", cmd_get_services);  // alias
    register_command("kubectl get configmaps", cmd_get_configmaps);
    register_command("kubectl get cm", cmd_get_configmaps);  // alias
    register_command("kubectl logs", cmd_logs);
    register_command("kubectl describe pod", cmd_describe_pod);
    register_command("kubectl delete pod", cmd_delete_pod);
    register_command("kubectl get secrets", cmd_get_secrets);
    register_command("kubectl create secret generic", cmd_create_secret_generic);
    register_command("kubectl get nodes", cmd_get_nodes);
    register_command("kubectl describe node", cmd_describe_node);
    register_command("kubectl top nodes", cmd_top_nodes);
    register_command("kubectl top pods", cmd_top_pods);
    register_command("kubectl get events", cmd_get_events);
    register_command("kubectl edit deployment", cmd_edit_deployment);
    register_command("kubectl scale deployment", cmd_scale);
    register_command("kubectl scale deploy", cmd_scale);  // alias
    register_command("kubectl rollout", cmd_rollout);
    register_command("kubectl cordon", cmd_cordon);
    register_command("kubectl uncordon", cmd_uncordon);
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

auto KubernetesModule::find_secret(const std::string& secret_name) -> std::vector<Secret>::iterator {
    return std::find_if(secrets.begin(), secrets.end(), [&](const Secret& s){ return s.name == secret_name; });
}

auto KubernetesModule::pod_end() -> decltype(pods.end()) {
    return decltype(nodes[0].pods.begin()){};  // Return sentinel for "not found"
}

auto KubernetesModule::secret_end() -> decltype(secrets.end()) {
    return secrets.end();
}

// Check if creating a secret triggers any quest actions
std::string KubernetesModule::check_secret_trigger(const std::string& secret_name) {
    if (active_quest_id.empty() || quest_data.count(active_quest_id) == 0) {
        return "";
    }

    const auto& quest = quest_data[active_quest_id];
    if (!quest["triggers"]) {
        return "";
    }

    for (const auto& trigger : quest["triggers"]) {
        if (trigger["type"] && trigger["type"].as<std::string>() == "resource_created") {
            std::string resource_type = trigger["resource"].as<std::string>();
            std::string resource_name = trigger["name"].as<std::string>();

            if (resource_type == "secret" && resource_name == secret_name) {
                if (trigger["actions"]) {
                    execute_actions(trigger["actions"]);

                    // Check if quest is now complete
                    return check_quest_completion();
                }
            }
        }
    }

    return "";
}

bool KubernetesModule::supports_command(const std::string& cmdPrefix) const {
    return command_registry.find(cmdPrefix) != command_registry.end();
}

std::string KubernetesModule::run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) {
    auto it = command_registry.find(cmdPrefix);
    if (it == command_registry.end()) {
        return "Command supported but not implemented in module\n";
    }

    // Store the full command that was executed (for quest checking)
    last_command_executed = cmdPrefix;
    if (!args.empty()) {
        for (const auto& arg : args) {
            last_command_executed += " " + arg;
        }
    }

    // Execute the command
    std::string result = it->second(args);

    // Check if this command completes the active quest
    std::string quest_completion = check_quest_completion();
    if (!quest_completion.empty()) {
        result += "\n" + quest_completion;
    }

    return result;
}

bool KubernetesModule::evaluate_condition(const YAML::Node& conditionSpec) {
    if (!conditionSpec || !conditionSpec["type"]) return false;
    std::string t = conditionSpec["type"].as<std::string>();

    // pod_status: Check if a pod has a specific status
    if (t == "pod_status") {
        std::string name = conditionSpec["pod"].as<std::string>();
        std::string expect = conditionSpec["status"].as<std::string>();
        auto it = find_pod(name);
        if (it == pod_end()) return false;
        return it->status == expect;
    }

    // command_run: Check if a specific command was executed
    else if (t == "command_run") {
        std::string expected_cmd = conditionSpec["command"].as<std::string>();
        return last_command_executed == expected_cmd;
    }

    // resource_exists: Check if a resource exists (configmap, secret, networkpolicy, etc.)
    else if (t == "resource_exists") {
        std::string resource = conditionSpec["resource"].as<std::string>();
        std::string name = conditionSpec["name"].as<std::string>();

        if (resource == "configmap") {
            auto it = std::find_if(configmaps.begin(), configmaps.end(),
                [&](const ConfigMap& cm) { return cm.name == name; });
            return it != configmaps.end();
        }
        else if (resource == "secret") {
            auto it = find_secret(name);
            return it != secret_end();
        }
        else if (resource == "networkpolicy" || resource == "ingress") {
            // NetworkPolicies and Ingress aren't currently stored, so assume created if checked
            // TODO: Add proper storage for these resources
            return true;
        }
        return false;
    }

    // deployment_replicas: Check if deployment has specific replica count
    else if (t == "deployment_replicas") {
        std::string name = conditionSpec["deployment"].as<std::string>();
        auto it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == name; });
        if (it == deployments.end()) return false;

        if (conditionSpec["min_replicas"] && conditionSpec["max_replicas"]) {
            int min = conditionSpec["min_replicas"].as<int>();
            int max = conditionSpec["max_replicas"].as<int>();
            return it->replicas >= min && it->replicas <= max;
        }
        else if (conditionSpec["min_replicas"]) {
            int min = conditionSpec["min_replicas"].as<int>();
            return it->replicas >= min;
        }
        return false;
    }

    // deployment_ready: Check if deployment is fully ready
    else if (t == "deployment_ready") {
        std::string name = conditionSpec["deployment"].as<std::string>();
        auto it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == name; });
        if (it == deployments.end()) return false;
        return it->ready_replicas == it->replicas && it->ready_replicas > 0;
    }

    // deployment_running: Check if deployment exists and has running pods
    else if (t == "deployment_running") {
        std::string name = conditionSpec["deployment"].as<std::string>();
        auto it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == name; });
        if (it == deployments.end()) return false;
        return it->available_replicas > 0;
    }

    // pod_restarts: Check if pod restart count meets criteria
    else if (t == "pod_restarts") {
        std::string name = conditionSpec["pod"].as<std::string>();
        auto it = find_pod(name);
        if (it == pod_end()) return false;

        if (conditionSpec["max_restarts"]) {
            int max = conditionSpec["max_restarts"].as<int>();
            return it->restarts <= max;
        }
        return false;
    }

    // service_has_endpoints: Check if service has endpoints (not implemented yet, stub)
    else if (t == "service_has_endpoints") {
        // This would require endpoint tracking - stub for now
        return false;
    }

    // deployment_rollout: Check deployment rollout status (not implemented yet, stub)
    else if (t == "deployment_rollout") {
        // This would require rollout history tracking - stub for now
        return false;
    }

    // ingress_routes_correctly: Check ingress routing
    else if (t == "ingress_routes_correctly") {
        std::string ingress_name = conditionSpec["ingress"].as<std::string>();

        // Find the ingress
        auto ing_it = std::find_if(ingresses.begin(), ingresses.end(),
            [&](const Ingress& ing) { return ing.name == ingress_name; });

        if (ing_it == ingresses.end()) return false;

        // Check required routes
        if (!conditionSpec["required_routes"] || !conditionSpec["required_routes"].IsSequence()) {
            return false;
        }

        for (const auto& req_route : conditionSpec["required_routes"]) {
            std::string req_host = req_route["host"].as<std::string>();
            std::string req_path = req_route["path"].as<std::string>();
            std::string req_service = req_route["service"].as<std::string>();

            // Find matching rule in ingress
            bool found = false;
            for (const auto& rule : ing_it->rules) {
                if (rule.host == req_host && rule.path == req_path &&
                    rule.service_name == req_service) {
                    found = true;
                    break;
                }
            }

            if (!found) return false;  // Required route not found
        }

        return true;  // All required routes present
    }

    // node_cordoned: Check if node is cordoned
    else if (t == "node_cordoned") {
        std::string node_name = conditionSpec["node"].as<std::string>();
        auto node_it = find_node(node_name);
        if (node_it == nodes.end()) return false;

        // Check if node has a cordoned state (would be in metadata or status)
        // For now, check if node has zero pods (indicating it was drained/cordoned)
        return node_it->pods.empty();
    }

    // multi_pod_health: Check multiple pods health
    else if (t == "multi_pod_health") {
        if (!conditionSpec["pods"] || !conditionSpec["pods"].IsSequence()) return false;

        bool all_healthy = conditionSpec["all_healthy"] && conditionSpec["all_healthy"].as<bool>();

        for (const auto& pod_node : conditionSpec["pods"]) {
            std::string pod_name = pod_node.as<std::string>();
            auto it = find_pod(pod_name);

            if (it == pod_end()) return false;  // Pod not found

            if (all_healthy) {
                // All pods must be Running
                if (it->status != "Running") {
                    return false;
                }
            }
        }

        return true;
    }

    // pvc_status: Check PVC status
    else if (t == "pvc_status") {
        // PVCs aren't tracked yet, but we can check if pods using them are healthy
        // For now, return true if the condition is checked (indicates PVC was created)
        return true;
    }

    // pod_event: Check for specific pod events
    else if (t == "pod_event") {
        std::string pod_name = conditionSpec["pod"].as<std::string>();
        std::string event_reason = conditionSpec["reason"].as<std::string>();

        auto it = find_pod(pod_name);
        if (it == pod_end()) return false;

        // Check if pod has the specified event
        for (const auto& event : it->events) {
            if (event.reason == event_reason) {
                return true;
            }
        }

        return false;
    }

    // service_has_endpoints: Check if service has endpoints
    else if (t == "service_has_endpoints") {
        std::string service_name = conditionSpec["service"].as<std::string>();

        // Find the service
        auto svc_it = std::find_if(services.begin(), services.end(),
            [&](const Service& s) { return s.name == service_name; });

        if (svc_it == services.end()) return false;

        // Check if there are pods matching the service selector
        // For now, we'll assume if the service exists and has a selector, it has endpoints
        int min_endpoints = conditionSpec["min_endpoints"] ? conditionSpec["min_endpoints"].as<int>() : 1;

        // Count pods matching service selector
        int endpoint_count = 0;
        for (const auto& node : nodes) {
            for (const auto& pod : node.pods) {
                if (pod.status == "Running") {
                    endpoint_count++;
                    if (endpoint_count >= min_endpoints) return true;
                }
            }
        }

        return endpoint_count >= min_endpoints;
    }

    // deployment_rollout: Check deployment rollout status
    else if (t == "deployment_rollout") {
        std::string deployment_name = conditionSpec["deployment"].as<std::string>();
        bool successful = conditionSpec["successful"] && conditionSpec["successful"].as<bool>();

        auto dep_it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == deployment_name; });

        if (dep_it == deployments.end()) return false;

        if (successful) {
            // Successful rollout means all replicas are ready
            return dep_it->ready_replicas == dep_it->replicas && dep_it->replicas > 0;
        }

        return true;
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

// Get a nested field from YAML using dot notation (e.g., "spec.template.spec.containers[0].env[0].valueFrom.secretKeyRef.name")
YAML::Node KubernetesModule::get_yaml_field(const YAML::Node& node, const std::string& path) {
    std::istringstream iss(path);
    std::string token;
    YAML::Node current = node;

    while (std::getline(iss, token, '.')) {
        if (token.empty()) continue;

        // Handle array indexing like "containers[0]"
        size_t bracket_pos = token.find('[');
        if (bracket_pos != std::string::npos) {
            std::string key = token.substr(0, bracket_pos);
            size_t idx = std::stoi(token.substr(bracket_pos + 1, token.find(']') - bracket_pos - 1));

            if (!current[key] || !current[key].IsSequence() || current[key].size() <= idx) {
                return YAML::Node();  // Return undefined node
            }
            current = current[key][idx];
        } else {
            if (!current[token]) {
                return YAML::Node();  // Return undefined node
            }
            current = current[token];
        }
    }

    return current;
}

// Validate edited YAML against quest rules
bool KubernetesModule::validate_edit(const std::string& original_yaml, const std::string& edited_yaml) {
    if (!active_quest_id.empty() && quest_data.count(active_quest_id) > 0) {
        const auto& quest = quest_data[active_quest_id];

        if (!quest["edit_validations"]) {
            std::cout << "deployment.apps/" << quest["deployment_template"]["pod"].as<std::string>() << " edited\n";
            return true;  // No validations defined
        }

        try {
            YAML::Node original = YAML::Load(original_yaml);
            YAML::Node edited = YAML::Load(edited_yaml);

            // Check each validation rule
            for (const auto& validation : quest["edit_validations"]) {
                std::string type = validation["type"].as<std::string>();

                if (type == "yaml_field_changed") {
                    std::string path_str = validation["path"].as<std::string>();
                    std::string old_value = validation["old_value"].as<std::string>();

                    YAML::Node original_field = get_yaml_field(original, path_str);
                    YAML::Node edited_field = get_yaml_field(edited, path_str);

                    if (!edited_field.IsDefined()) {
                        if (validation["on_failure"]) {
                            std::cout << validation["on_failure"]["feedback"].as<std::string>() << "\n";
                        }
                        return false;
                    }

                    std::string edited_value = edited_field.as<std::string>();

                    // Check if value changed correctly
                    if (validation["new_pattern"]) {
                        std::string pattern = validation["new_pattern"].as<std::string>();
                        std::regex regex_pattern(pattern);

                        if (std::regex_match(edited_value, regex_pattern) && edited_value != old_value) {
                            // Success! Execute actions
                            if (validation["on_success"]) {
                                execute_actions(validation["on_success"]["actions"]);
                                if (validation["on_success"]["feedback"]) {
                                    std::cout << validation["on_success"]["feedback"].as<std::string>() << "\n";
                                }
                            }
                            return true;
                        }
                    }

                    // Validation failed
                    if (validation["on_failure"]) {
                        std::cout << validation["on_failure"]["feedback"].as<std::string>() << "\n";
                    }
                    return false;
                }
            }

        } catch (const YAML::Exception& e) {
            std::cout << "Error parsing YAML: " << e.what() << "\n";
            return false;
        }
    }

    // No active quest, just report success
    std::cout << "deployment edited\n";
    return true;
}

// Execute actions defined in quest YAML
void KubernetesModule::execute_actions(const YAML::Node& actions) {
    if (!actions || !actions.IsSequence()) return;

    for (const auto& action : actions) {
        std::string type = action["type"].as<std::string>();

        if (type == "modify_pod") {
            std::string pod_name = action["pod"].as<std::string>();
            auto it = find_pod(pod_name);

            if (it != decltype(nodes[0].pods.begin()){}) {
                const auto& changes = action["changes"];
                if (changes["status"]) it->status = changes["status"].as<std::string>();
                if (changes["restarts"]) it->restarts = changes["restarts"].as<int>();
                if (changes["container_state"]) it->container_state = changes["container_state"].as<std::string>();
            }
        }
        else if (type == "clear_events") {
            std::string pod_name = action["pod"].as<std::string>();
            auto it = find_pod(pod_name);
            if (it != decltype(nodes[0].pods.begin()){}) {
                it->events.clear();
            }
        }
        else if (type == "clear_logs") {
            std::string pod_name = action["pod"].as<std::string>();
            auto it = find_pod(pod_name);
            if (it != decltype(nodes[0].pods.begin()){}) {
                it->logs.clear();
            }
        }
        else if (type == "add_log") {
            std::string pod_name = action["pod"].as<std::string>();
            auto it = find_pod(pod_name);
            if (it != decltype(nodes[0].pods.begin()){}) {
                PodLog log;
                log.timestamp = action["log"]["timestamp"].as<std::string>();
                log.message = action["log"]["message"].as<std::string>();
                it->logs.push_back(log);
            }
        }
        else if (type == "add_event") {
            std::string pod_name = action["pod"].as<std::string>();
            auto it = find_pod(pod_name);
            if (it != decltype(nodes[0].pods.begin()){}) {
                PodEvent event;
                event.type = action["event"]["type"].as<std::string>();
                event.reason = action["event"]["reason"].as<std::string>();
                event.message = action["event"]["message"].as<std::string>();
                event.timestamp = action["event"]["timestamp"].as<std::string>();
                it->events.push_back(event);
            }
        }
    }
}

// Check if quest is complete and return completion message
std::string KubernetesModule::check_quest_completion() {
    if (active_quest_id.empty() || quest_completed) {
        return "";  // No active quest or already completed
    }

    if (quest_data.count(active_quest_id) == 0) {
        return "";  // Quest data not loaded
    }

    const auto& quest = quest_data[active_quest_id];

    // Check if quest has a completion condition
    if (!quest["condition"]) {
        return "";
    }

    // Evaluate the completion condition
    if (evaluate_condition(quest["condition"])) {
        quest_completed = true;

        // Get the completion message from YAML
        std::string completion_msg = "Quest completed!";
        if (quest["completion_message"]) {
            completion_msg = quest["completion_message"].as<std::string>();
        }

        // Wrap it in epic SMITE celebration
        return QuestManager::quest_accomplished(completion_msg);
    }

    return "";  // Quest not yet complete
}

// Factory
std::shared_ptr<SmiteModule> create_module_kubernetes() {
    return std::make_shared<KubernetesModule>();
}
