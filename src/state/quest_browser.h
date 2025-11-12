#pragma once
#include "quest.h"
#include <string>
#include <vector>
#include <map>

// Forward declaration
class Router;

class QuestBrowser {
public:
    QuestBrowser(const QuestManager& quest_mgr, Router& router);

    // Launch interactive TUI browser
    // Returns the selected quest in format "module:quest_id" or empty string if cancelled
    std::string launch();

private:
    const QuestManager& quest_mgr_;
    Router& router_;

    struct ModuleInfo {
        std::string name;
        int total_quests;
        std::map<QuestDifficulty, std::vector<Quest>> quests_by_difficulty;
    };

    std::vector<ModuleInfo> gather_module_info();
    std::string difficulty_to_string(QuestDifficulty diff) const;
    int get_difficulty_count(const ModuleInfo& mod, QuestDifficulty diff) const;
};
