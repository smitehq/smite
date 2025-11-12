#include "globals.h"
#include "utils.h"
#include "engine.h"
#include "shell/shell.h"
#include <filesystem>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>
#include "state/quest.h"
#include "state/quest_browser.h"

namespace fs = std::filesystem;

// --------------------------
// Engine Constructor
// --------------------------
Engine::Engine(const std::string& modulesDir) 
    : modules_dir_(modulesDir), router_(), quests_(modulesDir), should_quit_(false) {
    
    quests_.load_all_quests();

    // Register default engine commands - NO EXCEPTIONS for control flow
    register_command("quit", [this](const auto&) -> std::string { 
        should_quit_ = true;
        return "Goodbye!\n";
    });
    
    register_command("exit", [this](const auto&) -> std::string { 
        should_quit_ = true;
        return "Goodbye!\n";
    });

    register_command("help", [this](const auto&) -> std::string {
        std::ostringstream oss;
        oss << "Registered command prefixes:\n";
        for (const auto& c : router_.list_commands()) {
            oss << "  " << c << "\n";
        }
        oss << "Other engine commands: modules, quests, hint, quit\n";
        return oss.str();
    });

    register_command("modules", [this](const auto&) -> std::string {
        std::ostringstream oss;
        oss << "Modules loaded:\n";
        for (const auto& mod : router_.get_modules()) {
            oss << "  - " << mod->name()
                << " (commands: " << mod->registered_prefixes().size() << ")\n";
        }
        return oss.str();
    });

    register_command("hint", [this](const auto&) -> std::string {
        // Find which module has an active quest
        std::string active_module;
        for (const auto& mod : router_.get_modules()) {
            // Check if this module has an active quest
            auto quests = quests_.get_quests_for_module(mod->name());
            for (const auto& q : quests) {
                if (quests_.is_active(mod->name(), q.id)) {
                    active_module = mod->name();
                    break;
                }
            }
            if (!active_module.empty()) break;
        }

        if (active_module.empty()) {
            return "No active quest. Activate a quest first with: quests <module> <quest_id>\n";
        }

        return quests_.get_next_hint(active_module);
    });

    register_command("quests", [this](const auto& args) -> std::string {
        if (args.size() < 2) {
            // Launch interactive TUI browser
            QuestBrowser browser(quests_, router_);
            std::string selected = browser.launch();

            if (selected.empty()) {
                return "";  // Cancelled
            }

            // Parse module:quest_id
            size_t colon_pos = selected.find(':');
            if (colon_pos == std::string::npos) {
                return "Error: Invalid quest selection\n";
            }

            std::string mod = selected.substr(0, colon_pos);
            std::string quest_id = selected.substr(colon_pos + 1);

            // Activate the quest
            const Quest* quest = quests_.get_quest(mod, quest_id);
            if (!quest) {
                return "Quest not found.\n";
            }

            if (!quests_.activate_quest(mod, quest_id)) {
                return "Failed to activate quest.\n";
            }

            auto mod_ptr = router_.get_module_by_name(mod);
            if (!mod_ptr) {
                return fmt::format("Module not found: {}\n", mod);
            }
            mod_ptr->activate_quest(quest_id);

            // Display epic quest activation message
            std::string output = "\n";
            output += "========================================================================\n";
            output += "                   ** QUEST ACTIVATED **                               \n";
            output += "========================================================================\n";
            output += "\n";
            output += fmt::format("  >> {}\n", fmt::styled(quest->title, globals::style::header));
            output += "  ----------------------------------------------------------------------\n";
            output += "\n";

            if (!quest->intro_text.empty()) {
                output += quest->intro_text + "\n\n";
            }

            output += "  Use 'hint' if you need guidance.\n\n";

            return output;
        }

        const auto& mod = args[1];
        if (args.size() == 2) {
            return quests_.list_quests(mod);
        }
        
        const auto& quest_id = args[2];

        // Get quest details for intro text
        const Quest* quest = quests_.get_quest(mod, quest_id);
        if (!quest) {
            return "Quest not found.\n";
        }

        // Activate in QuestManager
        if (!quests_.activate_quest(mod, quest_id)) {
            return "Failed to activate quest.\n";
        }

        // Activate in module
        auto mod_ptr = router_.get_module_by_name(mod);
        if (!mod_ptr) {
            return fmt::format("Module not found: {}\n", mod);
        }
        mod_ptr->activate_quest(quest_id);

        // Display epic quest activation message
        std::string output = "\n";
        output += "========================================================================\n";
        output += "                   ** QUEST ACTIVATED **                               \n";
        output += "========================================================================\n";
        output += "\n";
        output += fmt::format("  >> {}\n", fmt::styled(quest->title, globals::style::header));
        output += "  ----------------------------------------------------------------------\n";
        output += "\n";

        // Display intro text if available
        if (!quest->intro_text.empty()) {
            output += quest->intro_text + "\n\n";
        } else {
            output += "  " + quest->description + "\n\n";
        }

        output += "  ----------------------------------------------------------------------\n";
        output += fmt::format("  Reward: {}\n", fmt::styled("+" + std::to_string(quest->reward_xp) + " XP", globals::style::success));
        output += "\n";
        output += fmt::format("  {}\n", fmt::styled("May your debugging skills be sharp and your logs be clear.", globals::style::info));
        output += "\n";

        return output;
    });
}

// --------------------------
// Module management
// --------------------------
std::vector<std::string> Engine::discover_module_paths() const {
    std::vector<std::string> out;
    
    if (!fs::exists(modules_dir_) || !fs::is_directory(modules_dir_)) {
        std::cerr << "Warning: Modules directory not found: " << modules_dir_ << "\n";
        return out;
    }

    try {
        for (const auto& entry : fs::directory_iterator(modules_dir_)) {
            if (entry.is_directory()) {
                out.push_back(entry.path().string());
            }
        }
    } catch (const fs::filesystem_error& e) {
        std::cerr << "Error reading modules directory: " << e.what() << "\n";
    }
    
    return out;
}

bool Engine::load_modules() {
    auto module_paths = discover_module_paths();
    // Static factory approach (manual in main.cpp for now)
    // Future: dynamic loading with dlopen/LoadLibrary
    return !module_paths.empty();
}

void Engine::add_module(std::shared_ptr<SmiteModule> module) {
    if (!module) {
        std::cerr << "Error: Attempted to add null module\n";
        return;
    }
    router_.add_module(module);
}

// --------------------------
// Engine commands registry
// --------------------------
void Engine::register_command(const std::string& prefix, 
                              std::function<std::string(const std::vector<std::string>&)> handler) {
    if (prefix.empty()) {
        std::cerr << "Error: Cannot register command with empty prefix\n";
        return;
    }
    engine_commands_[prefix] = std::move(handler);
}

// --------------------------
// Command dispatch
// --------------------------
std::string Engine::dispatch_command(const std::string& cmd) {
    auto tokens = Utils::tokenize_command(cmd);
    if (tokens.empty()) return "";

    // Check engine commands first
    auto it = engine_commands_.find(tokens[0]);
    if (it != engine_commands_.end()) {
        return it->second(tokens);
    }

    // Otherwise, route to modules
    return router_.handle_command(tokens);
}

// --------------------------
// Shell module caching
// --------------------------
std::shared_ptr<Shell> Engine::get_shell_module() {
    if (!shell_cache_) {
        shell_cache_ = std::dynamic_pointer_cast<Shell>(get_module_by_name("shell"));
        if (!shell_cache_) {
            std::cerr << "Warning: Shell module not found\n";
        }
    }
    return shell_cache_;
}

// --------------------------
// REPL
// --------------------------
void Engine::repl() {
    // Load previous history
    read_history(".smite_history");

    // Setup for auto-completion
    auto shell = get_shell_module();
    if (shell) {
        shell->setup_readline_completion(&router_);
    }

    while (!should_quit_) {
        char* input = readline(get_prompt().c_str());
        if (!input) {
            // Ctrl+D pressed
            should_quit_ = true;
            std::cout << "\n";
            break;
        }
        
        std::string cmd = Utils::trim(input);
        if (!cmd.empty()) {
            add_history(input);
        }
        free(input);

        if (cmd.empty()) continue;

        // Handle multi-command chaining "&&"
        size_t and_pos = cmd.find(" && ");
        if (and_pos != std::string::npos) {
            std::string cmd1 = Utils::trim(cmd.substr(0, and_pos));
            std::string cmd2 = Utils::trim(cmd.substr(and_pos + 4));
            
            if (!cmd1.empty() && !cmd2.empty()) {
                std::string out1 = dispatch_command(cmd1);
                if (!out1.empty()) {
                    std::cout << out1;
                }
                
                // Only execute second command if first didn't error
                if (out1.find("Error") == std::string::npos &&
                    out1.find("Unknown") == std::string::npos &&
                    out1.find("not found") == std::string::npos) {
                    std::string out2 = dispatch_command(cmd2);
                    if (!out2.empty()) {
                        std::cout << out2;
                    }
                } else {
                    std::cout << " (Chain stopped on error)\n";
                }
                std::cout << std::endl;
                continue;
            }
        }

        // Normal dispatch
        std::string output = dispatch_command(cmd);
        if (!output.empty()) {
            std::cout << output;
        }
        std::cout << std::endl;
    }

    // Save history
    write_history(".smite_history");
    std::cout << "Exiting REPL.\n";
}

// --------------------------
// Helpers
// --------------------------


std::string Engine::get_prompt() {
    auto shell = get_shell_module();
    if (!shell) {
        return "$ ";  // Fallback prompt
    }

    std::string user = globals::PLAYER_NAME;
    std::string host = globals::HOSTNAME;
    std::string dir = shell->get_current_dir();
    
    if (dir == shell->get_home()) {
        dir = "~";
    }
    
    return fmt::format("\x1b[1;32m{}@{}\x1b[0m:\x1b[34m{}\x1b[0m$ ", user, host, dir);
}

// Expose means for module cross talk
std::shared_ptr<SmiteModule> Engine::get_module_by_name(const std::string& name) const {
    for (const auto& mod : router_.get_modules()) {
        if (mod->name() == name) {
            return mod;
        }
    }
    return nullptr;
}