#ifndef MODULES_KUBERNETES_SIMULATION_H
#define MODULES_KUBERNETES_SIMULATION_H

#include <string>
#include <vector>
#include <functional>
#include <yaml-cpp/yaml.h>
#include <chrono>

// Forward declaration
class KubernetesModule;

namespace k8s_simulation {

// ========================================
// Trigger Types
// ========================================

enum class TriggerType {
    INTERVAL,      // Execute every N seconds
    ONCE_AT,       // Execute once at specific elapsed time
    CONTINUOUS,    // Execute every tick while condition true
    POD_EVENT      // Execute when pod event occurs (Phase 3)
};

struct Trigger {
    TriggerType type;
    float interval_seconds = 0.0f;  // For INTERVAL type
    float at_time = 0.0f;           // For ONCE_AT type

    // For POD_EVENT type (Phase 3)
    std::string event_pod;
    std::string event_reason;

    static Trigger parse(const YAML::Node& node);
};

// ========================================
// Condition Types (for conditional simulation)
// ========================================

enum class ConditionType {
    POD_STATUS,
    POD_RESTARTS_ABOVE,
    POD_RESTARTS_BELOW,
    DEPLOYMENT_STATUS,
    QUEST_NOT_COMPLETED,
    METRIC_THRESHOLD,     // Phase 3: node metrics
    ALWAYS_TRUE
};

struct Condition {
    ConditionType type;
    std::string pod_name;
    std::string deployment_name;
    std::string status;
    int threshold = 0;

    // For METRIC_THRESHOLD (Phase 3)
    std::string node_name;
    std::string metric_name;
    std::string op;  // ">", "<", ">=", "<=", "=="
    float value = 0.0f;

    static Condition parse(const YAML::Node& node);
    bool evaluate(KubernetesModule* module) const;
};

// ========================================
// Action Types
// ========================================

enum class ActionType {
    INCREMENT_RESTARTS,
    ADD_LOG,
    ADD_EVENT,
    CYCLE_LOGS,
    UPDATE_POD_STATUS,
    MODIFY_DEPLOYMENT,
    CLEAR_EVENTS,
    CLEAR_LOGS,

    // Phase 3: Advanced actions
    CALCULATE_BACKOFF,
    INCREMENT_METRIC,
    WHEN_THRESHOLD,
    EVICT_POD,
    CASCADE_FAILURE
};

struct Action {
    ActionType type;
    std::string pod_name;
    std::string deployment_name;
    std::string node_name;

    // For INCREMENT_RESTARTS
    int max_value = 999;

    // For ADD_LOG
    std::string log_message;

    // For ADD_EVENT
    std::string event_type;       // "Warning", "Normal"
    std::string event_reason;     // "BackOff", "Unhealthy", etc.
    std::string event_message;

    // For CYCLE_LOGS
    std::vector<std::string> log_messages;
    int cycle_index = 0;

    // For UPDATE_POD_STATUS
    std::string new_status;
    std::string new_container_state;

    // For MODIFY_DEPLOYMENT
    std::string field_name;
    std::string field_value;

    // For INCREMENT_METRIC (Phase 3)
    std::string metric_name;
    float increment_by = 0.0f;
    float max_metric = 100.0f;

    // For WHEN_THRESHOLD (Phase 3)
    std::string threshold_metric;
    std::string threshold_op;
    float threshold_value = 0.0f;
    std::vector<Action> conditional_actions;  // Nested actions

    // For CASCADE_FAILURE (Phase 3)
    std::vector<std::string> dependent_pods;
    std::string cascade_status;
    std::string cascade_message;

    static Action parse(const YAML::Node& node);
    void execute(KubernetesModule* module, float delta_time);
};

// ========================================
// Simulation Rule
// ========================================

struct SimulationRule {
    std::string name;
    Trigger trigger;
    std::vector<Condition> conditions;  // All must be true
    std::vector<Action> actions;

    // Runtime state
    float timer = 0.0f;           // Accumulates time for interval triggers
    bool executed_once = false;   // For ONCE_AT triggers

    static SimulationRule parse(const YAML::Node& node);
    bool should_execute(float elapsed_time, float delta_time);
    void execute(KubernetesModule* module, float delta_time);
    void reset();
};

// ========================================
// Simulation Configuration
// ========================================

struct SimulationConfig {
    bool enabled = false;
    int tick_rate = 10;  // Updates per second (default 10 FPS)
    std::vector<SimulationRule> rules;

    static SimulationConfig parse(const YAML::Node& node);

    float get_tick_interval() const {
        return 1.0f / static_cast<float>(tick_rate);
    }
};

// ========================================
// Template Variable Replacement
// ========================================

class TemplateEngine {
public:
    static std::string replace_variables(
        const std::string& template_str,
        KubernetesModule* module,
        float elapsed_time,
        const std::string& pod_name = ""
    );

    static std::string get_current_timestamp();
    static int get_pod_restart_count(KubernetesModule* module, const std::string& pod_name);
};

} // namespace k8s_simulation

#endif // MODULES_KUBERNETES_SIMULATION_H
