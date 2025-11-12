#ifndef CORE_ENGINE_H
#define CORE_ENGINE_H

#include <string>
#include <memory>
#include <vector>
#include <unordered_map>
#include <functional>
#include "module_interface.h"
#include "router.h"
#include "state/quest.h"

// Forward declarations
class Shell;

class Engine {
public:
    explicit Engine(const std::string& modulesDir);

    // Module management
    std::vector<std::string> discover_module_paths() const;
    bool load_modules();  
    void add_module(std::shared_ptr<SmiteModule> module);

    // REPL
    void repl();

    // Command dispatch
    std::string dispatch_command(const std::string& cmd);

    // Engine command registration
    void register_command(const std::string& prefix, 
                         std::function<std::string(const std::vector<std::string>&)> handler);

    // Module cross talk
    std::shared_ptr<SmiteModule> get_module_by_name(const std::string& name) const;

    // Quest access
    const QuestManager& get_quest_manager() const { return quests_; }
    QuestManager& get_quest_manager_mutable() { return quests_; }
    Router& get_router() { return router_; }

private:
    std::string modules_dir_;
    Router router_;
    QuestManager quests_;
    bool should_quit_ = false;

    // Cached references for performance
    mutable std::shared_ptr<Shell> shell_cache_;

    // Engine commands
    std::unordered_map<std::string, 
                      std::function<std::string(const std::vector<std::string>&)>> engine_commands_;

    // Helpers
    std::string get_prompt();
    
    // Get shell module with caching
    std::shared_ptr<Shell> get_shell_module();
};

#endif // CORE_ENGINE_H