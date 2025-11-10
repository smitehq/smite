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
#include "quest.h"

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


    // Engine command registration
    void register_command(const std::string& prefix, std::function<std::string(const std::vector<std::string>&)> handler);

    // module cross talk
    std::shared_ptr<SmiteModule> get_module_by_name(const std::string &name) const;

private:
    std::string modules_dir;
    Router router;
    QuestManager quests;

    // Cached engine commands
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> engine_commands;

    // Helpers
    static std::string trim(const std::string& str);
    static void cout_flush(const std::string& msg);
    std::string get_prompt();
};

#endif
