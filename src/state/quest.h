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
    YAML::Node condition;
    int reward_xp;
};

class QuestManager {
public:
    explicit QuestManager(const std::string& modules_dir);

    // Loading
    void load_all_quests();
    std::vector<Quest> get_quests_for_module(const std::string& mod_name);

    // Activation
    bool activate_quest(const std::string& mod_name, const std::string& quest_id);
    bool is_active(const std::string& mod_name, const std::string& quest_id) const;

    // Persistence
    void save_state();
    void load_state();

    // String helpers
    std::string list_quests(const std::string& mod_name) const;

private:
    std::string modules_dir;
    std::map<std::string, std::vector<Quest>> quests_by_module;
    std::map<std::string, std::string> active_quest_per_module; // only one quest per module
};
