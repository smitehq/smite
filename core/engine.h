#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <filesystem>
#include <unordered_map>  // Add
#include <unordered_set>  // Add
#include "module_interface.h"  // SmiteModule forward if needed
#include "command_router.h"    // For CommandRouter

namespace fs = std::filesystem;

class Engine {
public:
    Engine(const std::string& modulesDir);
    std::vector<std::string> discover_module_paths() const;
    std::unordered_map<std::string, std::unordered_set<int>> active_quests;  // module_name -> set of quest IDs (0-indexed)
    bool load_modules();  // Future: Auto-call factories by convention
    void add_module(std::shared_ptr<SmiteModule> module);  // Plug-and-play add
    void repl();

private:
    std::string modules_dir;
    CommandRouter router;  // Owned router for dispatch
};

#endif