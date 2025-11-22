#include "module.h"
#include "simulation.h"
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
#include "commands/create_configmap.h"
#include "commands/edit_service.h"

namespace fs = std::filesystem;

// Constructor must be defined in .cpp where SimulationConfig is complete
KubernetesModule::KubernetesModule() = default;

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
    configmaps.clear();
    pvcs.clear();
    deployments.clear();
    statefulsets.clear();
    services.clear();
    ingresses.clear();
    network_policies.clear();

    if (!node["cluster"]) return;

    // Load nodes and pods
    if (node["cluster"]["nodes"]) {
        for (auto n : node["cluster"]["nodes"]) {
            Node node_struct;
            if (n["name"]) node_struct.name = n["name"].as<std::string>();
            if (n["ip"]) node_struct.ip = n["ip"].as<std::string>();
            if (n["cordoned"]) node_struct.cordoned = n["cordoned"].as<bool>();

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

                    if (p["labels"]) {
                        for (auto lbl : p["labels"]) {
                            pod.labels[lbl.first.as<std::string>()] = lbl.second.as<std::string>();
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

    // Load configmaps
    if (node["cluster"]["configmaps"]) {
        for (auto cm : node["cluster"]["configmaps"]) {
            ConfigMap configmap;
            if (cm["name"]) configmap.name = cm["name"].as<std::string>();
            if (cm["age"]) configmap.age = cm["age"].as<std::string>();
            if (cm["data"]) {
                for (auto kv : cm["data"]) {
                    configmap.data[kv.first.as<std::string>()] = kv.second.as<std::string>();
                }
            }
            configmaps.push_back(configmap);
        }
    }

    // Load PVCs
    if (node["cluster"]["pvcs"]) {
        for (auto pvc : node["cluster"]["pvcs"]) {
            PersistentVolumeClaim pvc_obj;
            if (pvc["name"]) pvc_obj.name = pvc["name"].as<std::string>();
            if (pvc["status"]) pvc_obj.status = pvc["status"].as<std::string>();
            if (pvc["volume_name"]) pvc_obj.volume_name = pvc["volume_name"].as<std::string>();
            if (pvc["storage_request"]) pvc_obj.storage_request = pvc["storage_request"].as<std::string>();
            if (pvc["storage_class"]) pvc_obj.storage_class = pvc["storage_class"].as<std::string>();
            if (pvc["age"]) pvc_obj.age = pvc["age"].as<std::string>();
            pvcs.push_back(pvc_obj);
        }
    }

    // Load deployments
    if (node["cluster"]["deployments"]) {
        for (auto d : node["cluster"]["deployments"]) {
            Deployment deployment;
            if (d["name"]) deployment.name = d["name"].as<std::string>();
            if (d["replicas"]) deployment.replicas = d["replicas"].as<int>();
            if (d["ready_replicas"]) deployment.ready_replicas = d["ready_replicas"].as<int>();
            if (d["available_replicas"]) deployment.available_replicas = d["available_replicas"].as<int>();
            if (d["image"]) deployment.image = d["image"].as<std::string>();
            if (d["age"]) deployment.age = d["age"].as<std::string>();
            if (d["revision"]) deployment.revision = d["revision"].as<int>();
            if (d["memory_limit"]) deployment.memory_limit = d["memory_limit"].as<std::string>();
            if (d["cpu_limit"]) deployment.cpu_limit = d["cpu_limit"].as<std::string>();
            if (d["liveness_initial_delay"]) deployment.liveness_initial_delay = d["liveness_initial_delay"].as<int>();
            if (d["liveness_timeout"]) deployment.liveness_timeout = d["liveness_timeout"].as<int>();
            if (d["affinity_type"]) deployment.affinity_type = d["affinity_type"].as<std::string>();
            if (d["labels"]) {
                for (auto lbl : d["labels"]) {
                    deployment.labels[lbl.first.as<std::string>()] = lbl.second.as<std::string>();
                }
            }
            deployments.push_back(deployment);
        }
    }

    // Load statefulsets
    if (node["cluster"]["statefulsets"]) {
        for (auto s : node["cluster"]["statefulsets"]) {
            StatefulSet statefulset;
            if (s["name"]) statefulset.name = s["name"].as<std::string>();
            if (s["replicas"]) statefulset.replicas = s["replicas"].as<int>();
            if (s["ready_replicas"]) statefulset.ready_replicas = s["ready_replicas"].as<int>();
            if (s["image"]) statefulset.image = s["image"].as<std::string>();
            if (s["age"]) statefulset.age = s["age"].as<std::string>();
            statefulsets.push_back(statefulset);
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

    // Load services
    if (node["cluster"]["services"]) {
        for (auto svc : node["cluster"]["services"]) {
            Service service;
            if (svc["name"]) service.name = svc["name"].as<std::string>();
            if (svc["type"]) service.type = svc["type"].as<std::string>();
            if (svc["cluster_ip"]) service.cluster_ip = svc["cluster_ip"].as<std::string>();
            if (svc["external_ip"]) service.external_ip = svc["external_ip"].as<std::string>();
            if (svc["age"]) service.age = svc["age"].as<std::string>();
            if (svc["ports"]) {
                for (auto port : svc["ports"]) {
                    service.ports.push_back(port.as<std::string>());
                }
            }
            if (svc["selector"]) {
                for (auto sel : svc["selector"]) {
                    service.selector[sel.first.as<std::string>()] = sel.second.as<std::string>();
                }
            }
            services.push_back(service);
        }
    }

    // Load network policies
    if (node["cluster"]["networkpolicies"]) {
        for (auto np : node["cluster"]["networkpolicies"]) {
            NetworkPolicy policy;
            if (np["name"]) policy.name = np["name"].as<std::string>();
            if (np["age"]) policy.age = np["age"].as<std::string>();
            network_policies.push_back(policy);
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

    // Load simulation configuration from state YAML (not quest YAML!)
    // Simulation describes the behavior of the broken cluster state
    if (fs::exists(quest_state_path)) {
        try {
            YAML::Node state = YAML::LoadFile(quest_state_path.string());
            if (state["simulation"]) {
                simulation_config_ = std::make_unique<k8s_simulation::SimulationConfig>(
                    k8s_simulation::SimulationConfig::parse(state["simulation"])
                );

                // Start simulation if enabled
                start_simulation();
            } else {
                // No simulation config in state, ensure simulation is stopped
                stop_simulation();
                simulation_config_.reset();
            }
        } catch (const std::exception& e) {
            std::cout << "Warning: Failed to load simulation config: " << e.what() << "\n";
            stop_simulation();
            simulation_config_.reset();
        }
    } else {
        // No state file, stop simulation
        stop_simulation();
        simulation_config_.reset();
    }

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
    register_command("kubectl create configmap", cmd_create_configmap);
    register_command("kubectl edit service", cmd_edit_service);
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
    // Lock state mutex to prevent race conditions with simulation thread
    std::lock_guard<std::mutex> lock(state_mutex_);

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
        else if (resource == "networkpolicy") {
            auto it = std::find_if(network_policies.begin(), network_policies.end(),
                [&](const NetworkPolicy& np) { return np.name == name; });
            return it != network_policies.end();
        }
        else if (resource == "ingress") {
            auto it = std::find_if(ingresses.begin(), ingresses.end(),
                [&](const Ingress& ing) { return ing.name == name; });
            return it != ingresses.end();
        }
        return false;
    }

    // resource_not_exists: Check if a resource does NOT exist (inverse of resource_exists)
    else if (t == "resource_not_exists") {
        std::string resource = conditionSpec["resource"].as<std::string>();
        std::string name = conditionSpec["name"].as<std::string>();

        if (resource == "deployment") {
            auto it = std::find_if(deployments.begin(), deployments.end(),
                [&](const Deployment& d) { return d.name == name; });
            return it == deployments.end();  // True if NOT found
        }
        else if (resource == "pod") {
            auto it = find_pod(name);
            return it == pod_end();  // True if NOT found
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

    // statefulset_replicas: Check if statefulset has specific replica count
    else if (t == "statefulset_replicas") {
        std::string name = conditionSpec["statefulset"].as<std::string>();
        auto it = std::find_if(statefulsets.begin(), statefulsets.end(),
            [&](const StatefulSet& s) { return s.name == name; });
        if (it == statefulsets.end()) return false;

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

    // deployment_image: Check if deployment uses a specific image (or doesn't use a bad one)
    else if (t == "deployment_image") {
        std::string name = conditionSpec["deployment"].as<std::string>();
        auto it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == name; });
        if (it == deployments.end()) return false;

        // Check if image does NOT match a pattern (e.g., not using broken-tag)
        if (conditionSpec["not_contains"]) {
            std::string not_contains = conditionSpec["not_contains"].as<std::string>();
            return it->image.find(not_contains) == std::string::npos;
        }
        // Check if image matches exactly
        else if (conditionSpec["image"]) {
            std::string expected = conditionSpec["image"].as<std::string>();
            return it->image == expected;
        }
        return false;
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

        // Check actual cordoned state if specified, otherwise check if node has zero pods
        if (conditionSpec["cordoned"]) {
            bool expected_cordon = conditionSpec["cordoned"].as<bool>();
            return node_it->cordoned == expected_cordon;
        }
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
        std::string pvc_name = conditionSpec["pvc"].as<std::string>();
        std::string expected_status = conditionSpec["status"].as<std::string>();

        auto pvc_it = std::find_if(pvcs.begin(), pvcs.end(),
            [&](const PersistentVolumeClaim& pvc) { return pvc.name == pvc_name; });

        if (pvc_it == pvcs.end()) return false;
        return pvc_it->status == expected_status;
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
        int min_endpoints = conditionSpec["min_endpoints"] ? conditionSpec["min_endpoints"].as<int>() : 1;

        // Count Running pods that match ALL selector labels
        int endpoint_count = 0;
        for (const auto& node : nodes) {
            for (const auto& pod : node.pods) {
                if (pod.status != "Running") continue;

                // Check if pod matches ALL service selector labels
                bool matches = true;
                for (const auto& selector_pair : svc_it->selector) {
                    auto pod_label = pod.labels.find(selector_pair.first);
                    if (pod_label == pod.labels.end() || pod_label->second != selector_pair.second) {
                        matches = false;
                        break;
                    }
                }

                if (matches) {
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

    // deployment_memory_limit: Check deployment memory limit meets minimum
    else if (t == "deployment_memory_limit") {
        std::string deployment_name = conditionSpec["deployment"].as<std::string>();

        auto dep_it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == deployment_name; });

        if (dep_it == deployments.end()) return false;
        if (dep_it->memory_limit.empty()) return false;

        // Check if memory limit meets minimum (e.g., "512Mi")
        if (conditionSpec["min_limit"]) {
            std::string min_limit = conditionSpec["min_limit"].as<std::string>();
            // Simple comparison: extract numeric value
            // "256Mi" -> 256, "1Gi" -> 1024
            auto parse_memory = [](const std::string& mem) -> int {
                if (mem.empty()) return 0;
                int value = std::stoi(mem);
                if (mem.find("Gi") != std::string::npos) return value * 1024;
                return value; // Assume Mi
            };

            int actual = parse_memory(dep_it->memory_limit);
            int required = parse_memory(min_limit);
            return actual >= required;
        }

        return true; // Has a memory limit set
    }

    // deployment_liveness_probe: Check deployment liveness probe configuration
    else if (t == "deployment_liveness_probe") {
        std::string deployment_name = conditionSpec["deployment"].as<std::string>();

        auto dep_it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == deployment_name; });

        if (dep_it == deployments.end()) return false;

        // Check if liveness probe is configured (delay > 0)
        if (conditionSpec["min_initial_delay"]) {
            int min_delay = conditionSpec["min_initial_delay"].as<int>();
            if (dep_it->liveness_initial_delay < min_delay) return false;
        }

        if (conditionSpec["min_timeout"]) {
            int min_timeout = conditionSpec["min_timeout"].as<int>();
            if (dep_it->liveness_timeout < min_timeout) return false;
        }

        // Just check probe exists
        return dep_it->liveness_initial_delay > 0 && dep_it->liveness_timeout > 0;
    }

    // deployment_affinity: Check deployment affinity configuration
    else if (t == "deployment_affinity") {
        std::string deployment_name = conditionSpec["deployment"].as<std::string>();

        auto dep_it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == deployment_name; });

        if (dep_it == deployments.end()) return false;

        // Check affinity type matches expected
        if (conditionSpec["affinity_type"]) {
            std::string expected_type = conditionSpec["affinity_type"].as<std::string>();
            return dep_it->affinity_type == expected_type;
        }

        // Check affinity is not "none"
        return !dep_it->affinity_type.empty() && dep_it->affinity_type != "none";
    }

    // service_has_endpoints: Check if a service has endpoints (pods matching its selector)
    else if (t == "service_has_endpoints") {
        std::string service_name = conditionSpec["service"].as<std::string>();

        auto svc_it = std::find_if(services.begin(), services.end(),
            [&](const Service& s) { return s.name == service_name; });

        if (svc_it == services.end()) return false;

        // Check if any pods match the service selector
        int matching_pods = 0;
        for (const auto& node : nodes) {
            for (const auto& pod : node.pods) {
                bool matches = true;
                for (const auto& [key, val] : svc_it->selector) {
                    auto label_it = pod.labels.find(key);
                    if (label_it == pod.labels.end() || label_it->second != val) {
                        matches = false;
                        break;
                    }
                }
                if (matches && pod.status == "Running") {
                    matching_pods++;
                }
            }
        }

        int min_endpoints = 1;
        if (conditionSpec["min_endpoints"]) {
            min_endpoints = conditionSpec["min_endpoints"].as<int>();
        }

        return matching_pods >= min_endpoints;
    }

    // deployment_rollback_completed: Check if deployment has been rolled back
    else if (t == "deployment_rollback_completed") {
        std::string deployment_name = conditionSpec["deployment"].as<std::string>();

        auto dep_it = std::find_if(deployments.begin(), deployments.end(),
            [&](const Deployment& d) { return d.name == deployment_name; });

        if (dep_it == deployments.end()) return false;

        // Check if revision decreased (indicating rollback) and deployment is healthy
        return dep_it->revision < 3 && dep_it->ready_replicas == dep_it->replicas;
    }

    // pod_has_multiple_containers: Check if pod has multiple containers
    else if (t == "pod_has_multiple_containers") {
        std::string pod_name = conditionSpec["pod"].as<std::string>();

        // For now, we'll simulate this by checking a pod field
        // In a real implementation, you'd track containers per pod
        int min_containers = 2;
        if (conditionSpec["min_containers"]) {
            min_containers = conditionSpec["min_containers"].as<int>();
        }

        // This would need to be tracked in the Pod struct
        // For now, return false (needs implementation)
        return false;  // TODO: Track containers in Pod struct
    }

    // node_cordoned: Check if a node is cordoned
    else if (t == "node_cordoned") {
        std::string node_name = conditionSpec["node"].as<std::string>();
        bool should_be_cordoned = true;
        if (conditionSpec["cordoned"]) {
            should_be_cordoned = conditionSpec["cordoned"].as<bool>();
        }

        auto node_it = find_node(node_name);
        if (node_it == nodes.end()) return false;

        return node_it->cordoned == should_be_cordoned;
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
        quest_just_completed = true;  // Set flag for engine to detect

        // Stop simulation when quest completes
        stop_simulation();

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

// ========================================
// Simulation System
// ========================================

KubernetesModule::~KubernetesModule() {
    stop_simulation();
}

void KubernetesModule::start_simulation() {
    // Stop any existing simulation
    stop_simulation();

    // Check if simulation is enabled in quest
    if (!simulation_config_ || !simulation_config_->enabled) {
        return;
    }

    // Reset quest start time
    quest_start_time_ = std::chrono::steady_clock::now();

    // Reset all rule timers
    for (auto& rule : simulation_config_->rules) {
        rule.reset();
    }

    // Start simulation thread
    simulation_running_ = true;
    simulation_thread_ = std::thread(&KubernetesModule::simulation_loop, this);
}

void KubernetesModule::stop_simulation() {
    if (simulation_running_) {
        simulation_running_ = false;
        if (simulation_thread_.joinable()) {
            simulation_thread_.join();
        }
    }
}

void KubernetesModule::simulation_loop() {
    using namespace std::chrono;

    const float tick_interval = simulation_config_->get_tick_interval();
    const auto tick_duration = duration_cast<milliseconds>(duration<float>(tick_interval));

    auto last_tick = steady_clock::now();

    while (simulation_running_) {
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - last_tick);
        float delta_time = elapsed.count() / 1000.0f;

        if (delta_time >= tick_interval) {
            float quest_elapsed = get_elapsed_time();

            // Lock state for the entire tick to ensure consistency
            {
                std::lock_guard<std::mutex> lock(state_mutex_);

                // Execute each rule
                for (auto& rule : simulation_config_->rules) {
                    if (rule.should_execute(quest_elapsed, delta_time)) {
                        rule.execute(this, delta_time);
                    }
                }
            }

            last_tick = now;
        }

        // Sleep for a short time to avoid busy-waiting
        std::this_thread::sleep_for(milliseconds(10));
    }
}

float KubernetesModule::get_elapsed_time() const {
    using namespace std::chrono;
    auto now = steady_clock::now();
    auto elapsed = duration_cast<milliseconds>(now - quest_start_time_);
    return elapsed.count() / 1000.0f;
}

// Factory
std::shared_ptr<SmiteModule> create_module_kubernetes() {
    return std::make_shared<KubernetesModule>();
}
