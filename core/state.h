#pragma once
#include <string>
#include <unordered_map>

struct EngineState {
    int player_xp = 0;
    std::unordered_map<std::string, bool> completed_quests;
    // add other generic fields as needed (inventory, badges, settings)
};
