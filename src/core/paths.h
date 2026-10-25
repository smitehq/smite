#pragma once
#include <filesystem>

namespace smite::paths {

// Absolute path of the running executable.
std::filesystem::path executable_path();

// Player save directory: ~/.smite (HOME, or USERPROFILE on Windows).
// Settings, readline history, and quest progress all live here.
std::filesystem::path user_state_dir();

// Module data directory (quests, cluster state).
// SMITE_DATA overrides. Otherwise search next to the executable, then walk
// parents for src/modules (or modules/) so a dist layout and a repo checkout both work.
std::filesystem::path modules_data_dir();

} // namespace smite::paths
