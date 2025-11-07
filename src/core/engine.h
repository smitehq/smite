#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <functional>
#include "module_interface.h"
#include "router.h"

class Engine {
public:
    Engine(const std::string& modulesDir);

    // Module management
    std::vector<std::string> discover_module_paths() const;
    bool load_modules();  
    void add_module(std::shared_ptr<SmiteModule> module);

    // REPL
    void repl();

    // Command dispatch
    std::string dispatch_command(const std::string& cmd);

    // Quest management
    void handle_quests(const std::vector<std::string>& tokens);

    // Engine command registration
    void register_command(const std::string& prefix, std::function<std::string(const std::vector<std::string>&)> handler);

private:
    std::string modules_dir;
    Router router;
    std::unordered_map<std::string, std::unordered_set<int>> active_quests;

    // Cached engine commands
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> engine_commands;

    std::string list_quests_for_module(const std::string& mod_name) const;

    // Helpers
    static std::string trim(const std::string& str);
    static void cout_flush(const std::string& msg);
};

#endif
