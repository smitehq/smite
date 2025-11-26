#include "telemetry.h"
#include <algorithm>

void TelemetryManager::start_quest_tracking(const std::string& module_name, const std::string& quest_id) {
    if (!telemetry_enabled) return;

    // Start new telemetry session for this module
    active_telemetry[module_name] = QuestTelemetry(module_name, quest_id);
}

void TelemetryManager::track_command(const std::string& module_name, const std::string& command, const std::string& full_input) {
    if (!telemetry_enabled) return;

    auto it = active_telemetry.find(module_name);
    if (it != active_telemetry.end()) {
        it->second.commands.emplace_back(command, full_input);
    }
}

void TelemetryManager::track_hint_used(const std::string& module_name) {
    if (!telemetry_enabled) return;

    auto it = active_telemetry.find(module_name);
    if (it != active_telemetry.end()) {
        it->second.hints_used++;
    }
}

void TelemetryManager::complete_quest_tracking(const std::string& module_name, const std::string& quest_id, int base_xp) {
    if (!telemetry_enabled) return;

    auto it = active_telemetry.find(module_name);
    if (it != active_telemetry.end()) {
        it->second.end_time = std::chrono::system_clock::now();
        it->second.completed = true;

        // Calculate total XP
        int duration_minutes = it->second.get_duration_minutes();
        int time_bonus = calculate_time_bonus(duration_minutes);
        it->second.xp_earned = base_xp + time_bonus;
    }
}

const QuestTelemetry* TelemetryManager::get_active_telemetry(const std::string& module_name) const {
    auto it = active_telemetry.find(module_name);
    if (it != active_telemetry.end()) {
        return &it->second;
    }
    return nullptr;
}

int TelemetryManager::calculate_time_bonus(int duration_minutes) const {
    // Base +100 XP bonus, decays by 5 per minute
    // Formula: max(0, 100 - (minutes * 5))
    int bonus = 100 - (duration_minutes * 5);
    return std::max(0, bonus);
}
