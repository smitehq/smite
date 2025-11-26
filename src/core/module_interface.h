#pragma once
#include <string>
#include <vector>

// forward-declare YAML::Node to avoid requiring yaml-cpp in code that only needs the interface:
namespace YAML { class Node; }

/*
  SmiteModule is the single interface the engine speaks to.
  The core engine is agnostic about domain concepts.
*/
class SmiteModule {
public:
    virtual ~SmiteModule() = default;

    // Module identity
    virtual std::string name() const = 0;

    // Called once at load time to let the module load its YAML/config from its folder
    // modulePath is the filesystem folder containing module files (commands.yaml, state.yaml, quests.yaml)
    virtual bool load_from_path(const std::string& modulePath) = 0;

    // Check if this module can handle the command prefix (like "kubectl get pods")
    // Engine will ask modules for matching before declaring "unknown command"
    virtual bool supports_command(const std::string& cmdPrefix) const = 0;

    // Execute a command; returns textual output (engine prints it). args contains the tokens after the matched prefix.
    virtual std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) = 0;

    // Additional module-specific methods can be added here
    virtual std::string list_quests() const { return "No quests available for this module.\n"; }

    // Activate a quest by ID; returns true if successful
    virtual bool activate_quest(const std::string& quest_id) { return false; }

    // Evaluate a quest condition. The engine will provide the condition YAML (module-specific schema).
    // Return true if condition satisfied by current module state.
    virtual bool evaluate_condition(const YAML::Node& conditionSpec) = 0;

    // Optional: list registered command prefixes for discovery/help
    virtual std::vector<std::string> registered_prefixes() const = 0;

    // Quest completion detection (for engine to detect and unlock next quests)
    virtual std::string get_active_quest_id() const { return ""; }
    virtual bool is_quest_active() const { return !get_active_quest_id().empty(); }
    virtual bool is_quest_just_completed() const { return false; }
    virtual void clear_quest_completion_flag() {}

    // Generate module-specific investigation tips based on telemetry
    // Forward declaration to avoid circular dependency
    virtual std::vector<std::string> generate_investigation_tips(const struct QuestTelemetry& telemetry) const { return {}; }
};
