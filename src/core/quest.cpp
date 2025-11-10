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
            q.condition = qyaml["condition"];
            q.reward_xp = qyaml["reward_xp"].as<int>(0);
            quests.push_back(q);
        }
        quests_by_module[mod_name] = quests;
    }
}

std::vector<Quest> QuestManager::get_quests_for_module(const std::string& mod) {
    return quests_by_module.count(mod) ? quests_by_module.at(mod) : std::vector<Quest>{};
}

bool QuestManager::activate_quest(const std::string& mod, const std::string& quest_id) {
    if (!quests_by_module.count(mod)) return false;
    for (const auto& q : quests_by_module[mod]) {
        if (q.id == quest_id) {
            active_quest_per_module[mod] = quest_id;
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
