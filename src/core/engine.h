#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include <unordered_set>
#include <functional>     // Add this for std::function
#include "module_interface.h"  // SmiteModule forward if needed
#include "command_router.h"    // For CommandRouter

namespace fs = std::filesystem;

class Engine {
public:
    Engine(const std::string& modulesDir);
    std::vector<std::string> discover_module_paths() const;
    bool load_modules();  // Future: Auto-call factories by convention
    void add_module(std::shared_ptr<SmiteModule> module);  // Plug-and-play add
    void repl();
    void handle_quests(const std::vector<std::string>& tokens);
    std::string dispatch_command(const std::string& cmd);

private:
    std::string modules_dir;
    CommandRouter router;  // Owned router for dispatch
    std::unordered_map<std::string, std::unordered_set<int>> active_quests;  // module_name -> set of quest IDs (0-indexed)
    std::string list_quests_for_module(const std::string& mod_name) const;
};

#endif