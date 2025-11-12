#include "simulation.h"
#include "module.h"
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <cmath>

namespace k8s_simulation {

// ========================================
// Trigger Parsing and Evaluation
// ========================================

Trigger Trigger::parse(const YAML::Node& node) {
    Trigger trigger;

    if (!node || !node["type"]) {
        trigger.type = TriggerType::CONTINUOUS;
        return trigger;
    }

    std::string type_str = node["type"].as<std::string>();

    if (type_str == "interval") {
        trigger.type = TriggerType::INTERVAL;
        if (node["every"]) {
            std::string every_str = node["every"].as<std::string>();
            // Parse "15s", "1m", "2.5s" etc
            if (every_str.back() == 's') {
                trigger.interval_seconds = std::stof(every_str.substr(0, every_str.size() - 1));
            } else if (every_str.back() == 'm') {
                trigger.interval_seconds = std::stof(every_str.substr(0, every_str.size() - 1)) * 60.0f;
            }
        }
    } else if (type_str == "once_at") {
        trigger.type = TriggerType::ONCE_AT;
        if (node["time"]) {
            std::string time_str = node["time"].as<std::string>();
            if (time_str.back() == 's') {
                trigger.at_time = std::stof(time_str.substr(0, time_str.size() - 1));
            }
        }
    } else if (type_str == "continuous") {
        trigger.type = TriggerType::CONTINUOUS;
    } else if (type_str == "pod_event") {
        trigger.type = TriggerType::POD_EVENT;
        if (node["pod"]) trigger.event_pod = node["pod"].as<std::string>();
        if (node["reason"]) trigger.event_reason = node["reason"].as<std::string>();
    }

    return trigger;
}

// ========================================
// Condition Parsing and Evaluation
// ========================================

Condition Condition::parse(const YAML::Node& node) {
    Condition cond;

    if (!node || !node["type"]) {
        cond.type = ConditionType::ALWAYS_TRUE;
        return cond;
    }

    std::string type_str = node["type"].as<std::string>();

    if (type_str == "pod_status") {
        cond.type = ConditionType::POD_STATUS;
        if (node["pod"]) cond.pod_name = node["pod"].as<std::string>();
        if (node["status"]) cond.status = node["status"].as<std::string>();
    } else if (type_str == "pod_restarts_above") {
        cond.type = ConditionType::POD_RESTARTS_ABOVE;
        if (node["pod"]) cond.pod_name = node["pod"].as<std::string>();
        if (node["threshold"]) cond.threshold = node["threshold"].as<int>();
    } else if (type_str == "pod_restarts_below") {
        cond.type = ConditionType::POD_RESTARTS_BELOW;
        if (node["pod"]) cond.pod_name = node["pod"].as<std::string>();
        if (node["threshold"]) cond.threshold = node["threshold"].as<int>();
    } else if (type_str == "deployment_status") {
        cond.type = ConditionType::DEPLOYMENT_STATUS;
        if (node["deployment"]) cond.deployment_name = node["deployment"].as<std::string>();
        if (node["status"]) cond.status = node["status"].as<std::string>();
    } else if (type_str == "quest_not_completed") {
        cond.type = ConditionType::QUEST_NOT_COMPLETED;
    } else if (type_str == "metric_threshold") {
        cond.type = ConditionType::METRIC_THRESHOLD;
        if (node["node"]) cond.node_name = node["node"].as<std::string>();
        if (node["metric"]) cond.metric_name = node["metric"].as<std::string>();
        if (node["operator"]) cond.op = node["operator"].as<std::string>();
        if (node["value"]) cond.value = node["value"].as<float>();
    } else {
        cond.type = ConditionType::ALWAYS_TRUE;
    }

    return cond;
}

bool Condition::evaluate(KubernetesModule* module) const {
    switch (type) {
        case ConditionType::ALWAYS_TRUE:
            return true;

        case ConditionType::POD_STATUS: {
            auto it = module->find_pod(pod_name);
            if (it == module->pod_end()) return false;
            return it->status == status;
        }

        case ConditionType::POD_RESTARTS_ABOVE: {
            auto it = module->find_pod(pod_name);
            if (it == module->pod_end()) return false;
            return it->restarts > threshold;
        }

        case ConditionType::POD_RESTARTS_BELOW: {
            auto it = module->find_pod(pod_name);
            if (it == module->pod_end()) return false;
            return it->restarts < threshold;
        }

        case ConditionType::DEPLOYMENT_STATUS: {
            auto& deployments = module->get_deployments_mutable();
            auto it = std::find_if(deployments.begin(), deployments.end(),
                [&](const Deployment& d) { return d.name == deployment_name; });
            if (it == deployments.end()) return false;
            return it->ready_replicas > 0;  // Simplified status check
        }

        case ConditionType::QUEST_NOT_COMPLETED:
            return !module->is_quest_completed();

        case ConditionType::METRIC_THRESHOLD:
            // Phase 3: Would check node metrics
            return false;

        default:
            return false;
    }
}

// ========================================
// Action Parsing and Execution
// ========================================

Action Action::parse(const YAML::Node& node) {
    Action action;

    if (!node || !node["type"]) {
        return action;
    }

    std::string type_str = node["type"].as<std::string>();

    if (type_str == "increment_restarts") {
        action.type = ActionType::INCREMENT_RESTARTS;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
        if (node["max"]) action.max_value = node["max"].as<int>();
    } else if (type_str == "add_log") {
        action.type = ActionType::ADD_LOG;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
        if (node["message"]) action.log_message = node["message"].as<std::string>();
    } else if (type_str == "add_event") {
        action.type = ActionType::ADD_EVENT;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
        if (node["event"]) {
            auto event = node["event"];
            if (event["type"]) action.event_type = event["type"].as<std::string>();
            if (event["reason"]) action.event_reason = event["reason"].as<std::string>();
            if (event["message"]) action.event_message = event["message"].as<std::string>();
        }
    } else if (type_str == "cycle_logs") {
        action.type = ActionType::CYCLE_LOGS;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
        if (node["messages"]) {
            for (const auto& msg : node["messages"]) {
                action.log_messages.push_back(msg.as<std::string>());
            }
        }
    } else if (type_str == "update_pod_status") {
        action.type = ActionType::UPDATE_POD_STATUS;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
        if (node["status"]) action.new_status = node["status"].as<std::string>();
        if (node["container_state"]) action.new_container_state = node["container_state"].as<std::string>();
    } else if (type_str == "clear_events") {
        action.type = ActionType::CLEAR_EVENTS;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
    } else if (type_str == "clear_logs") {
        action.type = ActionType::CLEAR_LOGS;
        if (node["pod"]) action.pod_name = node["pod"].as<std::string>();
    }
    // Phase 3 actions would be parsed here

    return action;
}

void Action::execute(KubernetesModule* module, float delta_time) {
    switch (type) {
        case ActionType::INCREMENT_RESTARTS: {
            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                if (it->restarts < max_value) {
                    it->restarts++;
                }
            }
            break;
        }

        case ActionType::ADD_LOG: {
            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                PodLog log;
                log.timestamp = TemplateEngine::get_current_timestamp();
                log.message = TemplateEngine::replace_variables(log_message, module, 0.0f, pod_name);

                it->logs.push_back(log);

                // Keep only last 100 logs to avoid memory issues
                if (it->logs.size() > 100) {
                    it->logs.erase(it->logs.begin());
                }
            }
            break;
        }

        case ActionType::ADD_EVENT: {
            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                PodEvent event;
                event.type = event_type;
                event.reason = event_reason;
                event.message = TemplateEngine::replace_variables(event_message, module, 0.0f, pod_name);
                event.timestamp = TemplateEngine::get_current_timestamp();

                it->events.push_back(event);

                // Keep only last 50 events
                if (it->events.size() > 50) {
                    it->events.erase(it->events.begin());
                }
            }
            break;
        }

        case ActionType::CYCLE_LOGS: {
            if (log_messages.empty()) break;

            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                PodLog log;
                log.timestamp = TemplateEngine::get_current_timestamp();
                log.message = log_messages[cycle_index % log_messages.size()];

                it->logs.push_back(log);

                // Keep only last 100 logs
                if (it->logs.size() > 100) {
                    it->logs.erase(it->logs.begin());
                }

                cycle_index++;
            }
            break;
        }

        case ActionType::UPDATE_POD_STATUS: {
            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                if (!new_status.empty()) {
                    it->status = new_status;
                }
                if (!new_container_state.empty()) {
                    it->container_state = new_container_state;
                }
            }
            break;
        }

        case ActionType::CLEAR_EVENTS: {
            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                it->events.clear();
            }
            break;
        }

        case ActionType::CLEAR_LOGS: {
            auto it = module->find_pod(pod_name);
            if (it != module->pod_end()) {
                it->logs.clear();
            }
            break;
        }

        default:
            break;
    }
}

// ========================================
// Simulation Rule
// ========================================

SimulationRule SimulationRule::parse(const YAML::Node& node) {
    SimulationRule rule;

    if (node["name"]) {
        rule.name = node["name"].as<std::string>();
    }

    if (node["trigger"]) {
        rule.trigger = Trigger::parse(node["trigger"]);
    }

    if (node["conditions"]) {
        for (const auto& cond_node : node["conditions"]) {
            rule.conditions.push_back(Condition::parse(cond_node));
        }
    }

    if (node["actions"]) {
        for (const auto& action_node : node["actions"]) {
            rule.actions.push_back(Action::parse(action_node));
        }
    }

    // Legacy support: single "condition" field
    if (node["condition"]) {
        rule.conditions.push_back(Condition::parse(node["condition"]));
    }

    return rule;
}

bool SimulationRule::should_execute(float elapsed_time, float delta_time) {
    // Check conditions first
    for (const auto& condition : conditions) {
        // Note: Actual evaluation happens in execute() with module access
        // This just checks timing-based execution
    }

    switch (trigger.type) {
        case TriggerType::INTERVAL:
            timer += delta_time;
            if (timer >= trigger.interval_seconds) {
                timer = 0.0f;  // Reset timer
                return true;
            }
            return false;

        case TriggerType::ONCE_AT:
            if (!executed_once && elapsed_time >= trigger.at_time) {
                executed_once = true;
                return true;
            }
            return false;

        case TriggerType::CONTINUOUS:
            return true;  // Execute every tick (conditions permitting)

        case TriggerType::POD_EVENT:
            // Phase 3: Would check if event occurred this tick
            return false;

        default:
            return false;
    }
}

void SimulationRule::execute(KubernetesModule* module, float delta_time) {
    // Evaluate all conditions
    for (const auto& condition : conditions) {
        if (!condition.evaluate(module)) {
            return;  // Condition failed, don't execute
        }
    }

    // All conditions passed, execute actions
    for (auto& action : actions) {
        action.execute(module, delta_time);
    }
}

void SimulationRule::reset() {
    timer = 0.0f;
    executed_once = false;
    for (auto& action : actions) {
        action.cycle_index = 0;
    }
}

// ========================================
// Simulation Configuration
// ========================================

SimulationConfig SimulationConfig::parse(const YAML::Node& node) {
    SimulationConfig config;

    if (!node) {
        return config;  // No simulation section
    }

    if (node["enabled"]) {
        config.enabled = node["enabled"].as<bool>();
    }

    if (node["tick_rate"]) {
        config.tick_rate = node["tick_rate"].as<int>();
    }

    if (node["rules"]) {
        for (const auto& rule_node : node["rules"]) {
            config.rules.push_back(SimulationRule::parse(rule_node));
        }
    }

    return config;
}

// ========================================
// Template Engine
// ========================================

std::string TemplateEngine::replace_variables(
    const std::string& template_str,
    KubernetesModule* module,
    float elapsed_time,
    const std::string& pod_name
) {
    std::string result = template_str;

    // Replace {{current_time}}
    size_t pos = result.find("{{current_time}}");
    if (pos != std::string::npos) {
        result.replace(pos, 16, get_current_timestamp());
    }

    // Replace {{elapsed_time}}
    pos = result.find("{{elapsed_time}}");
    if (pos != std::string::npos) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << elapsed_time;
        result.replace(pos, 16, oss.str());
    }

    // Replace {{restart_count}}
    pos = result.find("{{restart_count}}");
    if (pos != std::string::npos && !pod_name.empty()) {
        int restarts = get_pod_restart_count(module, pod_name);
        result.replace(pos, 17, std::to_string(restarts));
    }

    // Replace {{pod_name}}
    pos = result.find("{{pod_name}}");
    if (pos != std::string::npos) {
        result.replace(pos, 12, pod_name);
    }

    return result;
}

std::string TemplateEngine::get_current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    std::ostringstream oss;
    oss << std::put_time(std::gmtime(&time), "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

int TemplateEngine::get_pod_restart_count(KubernetesModule* module, const std::string& pod_name) {
    auto it = module->find_pod(pod_name);
    if (it == module->pod_end()) return 0;
    return it->restarts;
}

} // namespace k8s_simulation
