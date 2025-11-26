#pragma once
#include <string>
#include <vector>
#include <chrono>
#include <map>

// Represents a single command execution during a quest
struct CommandEvent {
    std::chrono::system_clock::time_point timestamp;
    std::string command;
    std::string full_input;  // Full command with arguments

    CommandEvent(const std::string& cmd, const std::string& input)
        : timestamp(std::chrono::system_clock::now())
        , command(cmd)
        , full_input(input) {}
};

// Telemetry data for a single quest session
struct QuestTelemetry {
    std::string quest_id;
    std::string module_name;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    std::vector<CommandEvent> commands;
    int hints_used = 0;
    bool completed = false;
    int xp_earned = 0;

    QuestTelemetry() = default;

    QuestTelemetry(const std::string& mod, const std::string& qid)
        : quest_id(qid)
        , module_name(mod)
        , start_time(std::chrono::system_clock::now()) {}

    // Calculate duration in seconds
    int get_duration_seconds() const {
        auto duration = end_time - start_time;
        return std::chrono::duration_cast<std::chrono::seconds>(duration).count();
    }

    // Calculate duration in minutes
    int get_duration_minutes() const {
        return (get_duration_seconds() + 59) / 60;  // Round up
    }
};

// Manages telemetry tracking across all modules
class TelemetryManager {
public:
    // Start tracking a new quest session
    void start_quest_tracking(const std::string& module_name, const std::string& quest_id);

    // Track a command execution
    void track_command(const std::string& module_name, const std::string& command, const std::string& full_input);

    // Track hint usage
    void track_hint_used(const std::string& module_name);

    // Complete quest tracking and calculate XP
    void complete_quest_tracking(const std::string& module_name, const std::string& quest_id, int base_xp);

    // Get current quest telemetry for a module
    const QuestTelemetry* get_active_telemetry(const std::string& module_name) const;

    // Calculate time-based bonus XP
    int calculate_time_bonus(int duration_minutes) const;

    // Settings
    void set_telemetry_enabled(bool enabled) { telemetry_enabled = enabled; }
    bool is_telemetry_enabled() const { return telemetry_enabled; }

private:
    // Active telemetry per module (only one active quest per module)
    std::map<std::string, QuestTelemetry> active_telemetry;

    // Settings
    bool telemetry_enabled = true;  // Default to enabled, user can opt-out
};
