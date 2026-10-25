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
#include "state/telemetry.h"
#include "state/settings.h"

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

    // Start k8s_first_steps when ~/.smite has no saved quest progress.
    // Returns the activation banner, or empty when this is not a first launch.
    std::string begin_first_quest_if_needed();

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

    // Telemetry access
    TelemetryManager& get_telemetry() { return telemetry_; }

    // Settings access
    SettingsManager& get_settings() { return settings_; }

    // Set when the command that just ran completed a quest. Clears the flag.
    bool consume_quest_celebration();

private:
    std::string modules_dir_;
    Router router_;
    QuestManager quests_;
    TelemetryManager telemetry_;
    SettingsManager settings_;
    bool should_quit_ = false;

    // Cached references for performance
    mutable std::shared_ptr<Shell> shell_cache_;

    // Engine commands
    std::unordered_map<std::string, 
                      std::function<std::string(const std::vector<std::string>&)>> engine_commands_;

    // Helpers
    std::string get_prompt();
    void prompt_telemetry_consent();
    void flush_telemetry_prompt();
    void play_quest_celebration();
    std::string activate_named_quest(const std::string& mod, const std::string& quest_id);

    bool pending_telemetry_prompt_ = false;
    bool quest_completed_this_command_ = false;
    bool first_quest_started_ = false;

    // Get shell module with caching
    std::shared_ptr<Shell> get_shell_module();
};

#endif // CORE_ENGINE_H