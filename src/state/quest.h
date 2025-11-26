#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <yaml-cpp/yaml.h>

enum class QuestDifficulty {
    BEGINNER,
    INTERMEDIATE,
    ADVANCED,
    EXPERT
};

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    std::string intro_text;  // Story/intro shown on activation
    std::string completion_message;  // Message shown on quest completion
    std::string root_cause;  // Root cause description for post-mortem
    std::vector<std::string> hints;  // Progressive hints
    YAML::Node condition;
    int reward_xp;
    QuestDifficulty difficulty = QuestDifficulty::BEGINNER;
    std::vector<std::string> unlock_quests;  // Quests unlocked on completion
};

class QuestManager {
public:
    explicit QuestManager(const std::string& modules_dir);

    // Loading
    void load_all_quests();
    std::vector<Quest> get_quests_for_module(const std::string& mod_name) const;
    const Quest* get_quest(const std::string& mod_name, const std::string& quest_id) const;

    // Activation
    bool activate_quest(const std::string& mod_name, const std::string& quest_id);
    bool is_active(const std::string& mod_name, const std::string& quest_id) const;

    // Quest progress
    void mark_quest_completed(const std::string& mod_name, const std::string& quest_id);
    bool is_quest_unlocked(const std::string& mod_name, const std::string& quest_id) const;
    bool is_quest_completed(const std::string& mod_name, const std::string& quest_id) const;

    // Hints
    std::string get_next_hint(const std::string& mod_name);
    int get_hints_shown(const std::string& mod_name) const;

    // Quest completion celebration (static so modules can call it directly)
    static std::string quest_accomplished(const std::string& completion_message);

    // Generate post-mortem report with telemetry analysis
    std::string generate_post_mortem(const std::string& mod_name, const std::string& quest_id, const class TelemetryManager& telemetry, const class SmiteModule* module) const;

    // Persistence
    void save_state();
    void load_state();

    // String helpers
    std::string list_quests(const std::string& mod_name) const;
    std::string list_all_quests_grouped() const;  // List all quests grouped by module and difficulty

private:
    std::string modules_dir;
    std::map<std::string, std::vector<Quest>> quests_by_module;
    std::map<std::string, std::string> active_quest_per_module; // only one quest per module
    std::map<std::string, int> hints_shown_per_module; // module -> number of hints shown
    std::map<std::string, std::set<std::string>> unlocked_quests_per_module; // module -> set of unlocked quest IDs
    std::map<std::string, std::set<std::string>> completed_quests_per_module; // module -> set of completed quest IDs
};
