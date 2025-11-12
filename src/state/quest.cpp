#include "quest.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

QuestManager::QuestManager(const std::string& dir) : modules_dir(dir) {}

void QuestManager::load_all_quests() {
    for (auto &p : fs::directory_iterator(modules_dir)) {
        if (!p.is_directory()) continue;
        auto mod_name = p.path().filename().string();
        std::string quests_dir = (p.path() / "quests").string();
        if (!fs::exists(quests_dir)) continue;

        std::vector<Quest> quests;
        for (auto &f : fs::directory_iterator(quests_dir)) {
            if (f.path().extension() != ".yaml") continue;
            YAML::Node qyaml = YAML::LoadFile(f.path().string());
            Quest q;
            q.id = qyaml["id"].as<std::string>();
            q.title = qyaml["title"].as<std::string>();
            q.description = qyaml["description"].as<std::string>();
            q.intro_text = qyaml["intro_text"].as<std::string>("");

            // Load difficulty
            std::string diff = qyaml["difficulty"].as<std::string>("beginner");
            if (diff == "beginner") q.difficulty = QuestDifficulty::BEGINNER;
            else if (diff == "intermediate") q.difficulty = QuestDifficulty::INTERMEDIATE;
            else if (diff == "advanced") q.difficulty = QuestDifficulty::ADVANCED;
            else if (diff == "expert") q.difficulty = QuestDifficulty::EXPERT;

            // Load hints
            if (qyaml["hints"] && qyaml["hints"].IsSequence()) {
                for (const auto& hint : qyaml["hints"]) {
                    q.hints.push_back(hint.as<std::string>());
                }
            }

            q.condition = qyaml["condition"];
            q.reward_xp = qyaml["reward_xp"].as<int>(0);
            quests.push_back(q);
        }
        quests_by_module[mod_name] = quests;
    }
}

std::vector<Quest> QuestManager::get_quests_for_module(const std::string& mod) const {
    return quests_by_module.count(mod) ? quests_by_module.at(mod) : std::vector<Quest>{};
}

const Quest* QuestManager::get_quest(const std::string& mod, const std::string& quest_id) const {
    auto it = quests_by_module.find(mod);
    if (it == quests_by_module.end()) return nullptr;

    for (const auto& q : it->second) {
        if (q.id == quest_id) {
            return &q;
        }
    }
    return nullptr;
}

bool QuestManager::activate_quest(const std::string& mod, const std::string& quest_id) {
    if (!quests_by_module.count(mod)) return false;
    for (const auto& q : quests_by_module[mod]) {
        if (q.id == quest_id) {
            active_quest_per_module[mod] = quest_id;
            hints_shown_per_module[mod] = 0;  // Reset hints when activating quest
            return true;
        }
    }
    return false;
}

bool QuestManager::is_active(const std::string& mod, const std::string& id) const {
    auto it = active_quest_per_module.find(mod);
    return it != active_quest_per_module.end() && it->second == id;
}

std::string QuestManager::list_quests(const std::string& mod_name) const {
    std::ostringstream oss;
    auto it = quests_by_module.find(mod_name);
    if (it == quests_by_module.end()) return "No quests found.\n";

    for (const auto& q : it->second) {
        oss << "  - " << q.id << ": " << q.title;
        if (is_active(mod_name, q.id)) oss << " [ACTIVE]";
        oss << "\n";
    }
    return oss.str();
}

std::string QuestManager::get_next_hint(const std::string& mod_name) {
    // Check if there's an active quest
    auto active_it = active_quest_per_module.find(mod_name);
    if (active_it == active_quest_per_module.end()) {
        return "No active quest for this module. Activate a quest first.\n";
    }

    // Get the active quest
    const Quest* quest = get_quest(mod_name, active_it->second);
    if (!quest) {
        return "Error: Active quest data not found.\n";
    }

    // Check if there are any hints
    if (quest->hints.empty()) {
        return "This quest has no hints available. You're on your own, adventurer!\n";
    }

    // Get current hint count
    int hints_shown = hints_shown_per_module[mod_name];

    // Check if all hints have been shown
    if (hints_shown >= static_cast<int>(quest->hints.size())) {
        std::ostringstream oss;
        oss << "\n";
        oss << "===================================================================\n";
        oss << "  ** No More Hints Available **\n";
        oss << "===================================================================\n\n";
        oss << "  You've exhausted all available wisdom. The rest of the journey\n";
        oss << "  is yours alone to walk. Trust in your skills, adventurer.\n\n";
        oss << "  Hints shown: " << hints_shown << "/" << quest->hints.size() << "\n\n";
        return oss.str();
    }

    // Show the next hint
    std::string hint_text = quest->hints[hints_shown];
    hints_shown_per_module[mod_name]++;

    // Format the hint beautifully
    std::ostringstream oss;
    oss << "\n";
    oss << "===================================================================\n";
    oss << "  ** Hint " << (hints_shown + 1) << " of " << quest->hints.size() << " **\n";
    oss << "===================================================================\n\n";
    oss << "  An ancient voice whispers from the void:\n\n";
    oss << "  \"" << hint_text << "\"\n\n";
    oss << "  -------------------------------------------------------------------\n\n";

    if (hints_shown + 1 < static_cast<int>(quest->hints.size())) {
        oss << "  Type 'hint' again if you need further guidance.\n\n";
    } else {
        oss << "  This was the final hint. The rest is up to you!\n\n";
    }

    return oss.str();
}

int QuestManager::get_hints_shown(const std::string& mod_name) const {
    auto it = hints_shown_per_module.find(mod_name);
    if (it == hints_shown_per_module.end()) return 0;
    return it->second;
}

std::string QuestManager::quest_accomplished(const std::string& completion_message) {
    std::ostringstream celebration;

    // // Confetti explosion!
    // celebration << "        *  .  *    .   *   .    *    .   *    .  *\n";
    // celebration << "     .    *   .  *   .   *   .   *   .  *   .    *\n";
    // celebration << "   *   .    *    .  *   .  *    .  *   .   *   .   *\n";
    // celebration << "     .  *   .  *    .  *   .  *   .   *  .   *   .\n";
    // celebration << "\n";

    // Victory banner
    celebration << "    \\O/   \\O/   \\O/   QUEST COMPLETE!   \\O/   \\O/   \\O/\n";
    celebration << "\n";

    // Add the custom completion message from YAML
    if (!completion_message.empty()) {
        celebration << completion_message << "\n";
    }

    // // Final confetti burst
    // celebration << "\n";
    // celebration << "  *  '  *  '  *  '  *  '  *  '  *  '  *  '  *  '  *  '\n";
    // celebration << "   '  *  '  *  '  *  '  *  '  *  '  *  '  *  '  *  '\n";
    // celebration << "  *  '  *  '  *  '  *  '  *  '  *  '  *  '  *  '  *  '\n";
    // celebration << "\n";

    return celebration.str();
}

std::string QuestManager::list_all_quests_grouped() const {
    std::ostringstream oss;
    
    auto diff_to_str = [](QuestDifficulty diff) -> std::string {
        switch (diff) {
            case QuestDifficulty::BEGINNER: return "Beginner";
            case QuestDifficulty::INTERMEDIATE: return "Intermediate";
            case QuestDifficulty::ADVANCED: return "Advanced";
            case QuestDifficulty::EXPERT: return "Expert";
            default: return "Unknown";
        }
    };
    
    oss << "\n";
    oss << "================================================================================\n";
    oss << "                              QUEST BROWSER                                     \n";
    oss << "================================================================================\n\n";
    
    for (const auto& [mod_name, quests] : quests_by_module) {
        if (quests.empty()) continue;
        
        // Count quests by difficulty
        std::map<QuestDifficulty, std::vector<const Quest*>> by_diff;
        for (const auto& q : quests) {
            by_diff[q.difficulty].push_back(&q);
        }
        
        // Module header
        oss << "  " << mod_name << " (" << quests.size() << " quests)\n";
        oss << "  " << std::string(mod_name.length() + 10, '=') << "\n\n";
        
        // List by difficulty
        for (auto diff : {QuestDifficulty::BEGINNER, QuestDifficulty::INTERMEDIATE, 
                          QuestDifficulty::ADVANCED, QuestDifficulty::EXPERT}) {
            if (by_diff.count(diff) == 0 || by_diff[diff].empty()) continue;
            
            oss << "    " << diff_to_str(diff) << " (" << by_diff[diff].size() << "):\n";
            for (const auto* q : by_diff[diff]) {
                std::string status = "";
                if (is_active(mod_name, q->id)) {
                    status = " [ACTIVE]";
                }
                oss << "      - " << q->id << ": " << q->title << " [" << q->reward_xp << " XP]" << status << "\n";
            }
            oss << "\n";
        }
        
        oss << "\n";
    }
    
    oss << "Use 'quest activate <module> <quest_id>' to start a quest\n";
    oss << "Use 'hint' during a quest to get guidance\n\n";
    
    return oss.str();
}
