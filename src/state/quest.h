#pragma once
#include <string>
#include <map>
#include <set>
#include <vector>
#include <yaml-cpp/yaml.h>

struct Quest {
    std::string id;
    std::string title;
    std::string description;
    std::string intro_text;  // Story/intro shown on activation
    std::vector<std::string> hints;  // Progressive hints
    YAML::Node condition;
    int reward_xp;
};

class QuestManager {
public:
    explicit QuestManager(const std::string& modules_dir);

    // Loading
    void load_all_quests();
    std::vector<Quest> get_quests_for_module(const std::string& mod_name);
    const Quest* get_quest(const std::string& mod_name, const std::string& quest_id) const;

    // Activation
    bool activate_quest(const std::string& mod_name, const std::string& quest_id);
    bool is_active(const std::string& mod_name, const std::string& quest_id) const;

    // Hints
    std::string get_next_hint(const std::string& mod_name);
    int get_hints_shown(const std::string& mod_name) const;

    // Quest completion celebration (static so modules can call it directly)
    static std::string quest_accomplished(const std::string& completion_message);

    // Persistence
    void save_state();
    void load_state();

    // String helpers
    std::string list_quests(const std::string& mod_name) const;

private:
    std::string modules_dir;
    std::map<std::string, std::vector<Quest>> quests_by_module;
    std::map<std::string, std::string> active_quest_per_module; // only one quest per module
    std::map<std::string, int> hints_shown_per_module; // module -> number of hints shown
};
