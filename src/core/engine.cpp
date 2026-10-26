#include "globals.h"
#include "utils.h"
#include "engine.h"
#include "confetti.h"
#include "paths.h"
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
    : modules_dir_(modulesDir), router_(), quests_(modulesDir), settings_(), should_quit_(false) {

    quests_.load_all_quests();
    quests_.load_state();  // Load saved quest progress

    // Apply telemetry settings
    telemetry_.set_telemetry_enabled(settings_.is_telemetry_enabled());

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
        oss << "Other engine commands: modules, quests, hint, settings, quit\n";
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

        // Track hint usage in telemetry
        telemetry_.track_hint_used(active_module);

        return quests_.get_next_hint(active_module);
    });

    register_command("settings", [this](const auto& args) -> std::string {
        if (args.size() == 1) {
            // Display current settings
            std::ostringstream oss;
            oss << "Current Settings:\n";
            oss << "  telemetry: " << (settings_.is_telemetry_enabled() ? "enabled" : "disabled") << "\n";
            oss << "\nUsage: settings telemetry <enable|disable>\n";
            return oss.str();
        }

        if (args.size() >= 3 && args[1] == "telemetry") {
            if (args[2] == "enable") {
                settings_.set_telemetry_enabled(true);
                telemetry_.set_telemetry_enabled(true);
                return "Telemetry enabled. Quest data will be tracked for post-mortem analysis.\n";
            } else if (args[2] == "disable") {
                settings_.set_telemetry_enabled(false);
                telemetry_.set_telemetry_enabled(false);
                return "Telemetry disabled. Quest data will not be tracked.\n";
            }
        }

        return "Usage: settings telemetry <enable|disable>\n";
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
            return activate_named_quest(mod, quest_id);
        }

        const auto& mod = args[1];
        if (args.size() == 2) {
            return quests_.list_quests(mod);
        }
        
        return activate_named_quest(mod, args[2]);
    });
}

std::string format_quest_activated(const Quest& quest) {
    std::string output = "\n";
    output += "========================================================================\n";
    output += "                   ** QUEST ACTIVATED **\n";
    output += "========================================================================\n";
    output += "\n";
    output += fmt::format("  >> {}\n", fmt::styled(quest.title, globals::style::header));
    output += "  ----------------------------------------------------------------------\n";
    output += "\n";

    if (!quest.intro_text.empty()) {
        output += quest.intro_text + "\n";
    } else {
        output += "  " + quest.description + "\n\n";
    }

    output += "  ----------------------------------------------------------------------\n";
    output += fmt::format("  Reward: {}\n", fmt::styled("+" + std::to_string(quest.reward_xp) + " XP", globals::style::success));
    output += "\n";
    output += "  Use 'hint' if you need guidance.\n";
    output += "\n";
    return output;
}

std::string Engine::activate_named_quest(const std::string& mod, const std::string& quest_id) {
    const Quest* quest = quests_.get_quest(mod, quest_id);
    if (!quest) {
        return "Quest not found.\n";
    }
    if (!quest->available) {
        return "That quest is not available yet. Its hints need kubectl commands smite does not implement.\n";
    }
    if (!quests_.is_quest_unlocked(mod, quest_id)) {
        return "That quest is locked.\n";
    }
    if (!quests_.activate_quest(mod, quest_id)) {
        return "Failed to activate quest.\n";
    }

    auto mod_ptr = router_.get_module_by_name(mod);
    if (!mod_ptr) {
        return fmt::format("Module not found: {}\n", mod);
    }
    mod_ptr->activate_quest(quest_id);
    telemetry_.start_quest_tracking(mod, quest_id);
    return format_quest_activated(*quest);
}

std::string Engine::begin_first_quest_if_needed() {
    if (first_quest_started_ || quests_.has_persisted_state()) {
        return "";
    }
    if (!router_.get_module_by_name("kubernetes")) {
        return "";
    }

    std::string banner = activate_named_quest("kubernetes", "k8s_first_steps");
    if (!quests_.is_active("kubernetes", "k8s_first_steps")) {
        return banner;
    }

    first_quest_started_ = true;
    quests_.save_state();
    return banner;
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

    // Track command for telemetry (for all active quests)
    for (const auto& mod : router_.get_modules()) {
        if (mod->is_quest_active()) {
            std::string command_name = tokens[0];
            telemetry_.track_command(mod->name(), command_name, cmd);
        }
    }

    // Otherwise, route to modules
    std::string result = router_.handle_command(tokens);

    // Check if any module just completed a quest
    for (const auto& mod : router_.get_modules()) {
        if (mod->is_quest_just_completed()) {
            std::string quest_id = mod->get_active_quest_id();
            std::string mod_name = mod->name();

            // Get quest info for XP
            const Quest* quest = quests_.get_quest(mod_name, quest_id);
            if (quest) {
                // Complete telemetry tracking
                telemetry_.complete_quest_tracking(mod_name, quest_id, quest->reward_xp);

                // Generate and append post-mortem report (pass module for tips generation)
                std::string post_mortem = quests_.generate_post_mortem(mod_name, quest_id, telemetry_, mod.get());
                result += post_mortem;
            }

            quests_.mark_quest_completed(mod_name, quest_id);
            mod->clear_quest_completion_flag();
            quest_completed_this_command_ = true;
            result += "\n";
            result += quests_.format_next_step(mod_name, quest_id);
            if (!settings_.has_telemetry_preference()) {
                pending_telemetry_prompt_ = true;
            }
        }
    }

    return result;
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
// Helper: Get History File Path
// --------------------------
std::string get_history_file_path() {
    return (smite::paths::user_state_dir() / ".history").string();
}

// --------------------------
// Telemetry Consent Prompt
// --------------------------
void Engine::prompt_telemetry_consent() {
    std::cout << "\n";
    std::cout << "========================================================================\n";
    std::cout << "                      Telemetry Settings                                \n";
    std::cout << "========================================================================\n";
    std::cout << "\n";
    std::cout << "Smite can track your quest progress to provide post-mortem analysis\n";
    std::cout << "after quest completion. This includes:\n";
    std::cout << "  - Commands executed during quests\n";
    std::cout << "  - Time taken to complete quests\n";
    std::cout << "  - Hints used\n";
    std::cout << "  - Investigation tips based on your approach\n";
    std::cout << "\n";
    std::cout << "All data is stored locally and never sent anywhere.\n";
    std::cout << "\n";
    std::cout << "Would you like to enable telemetry? (y/n): ";
    std::cout.flush();

    std::string response;
    std::getline(std::cin, response);
    response = Utils::trim(response);

    bool enabled = (response == "y" || response == "Y" || response == "yes" || response == "Yes" || response == "YES");

    settings_.set_telemetry_enabled(enabled);
    telemetry_.set_telemetry_enabled(enabled);

    std::cout << "\n";
    if (enabled) {
        std::cout << "Telemetry enabled. You can change this anytime with: settings telemetry disable\n";
    } else {
        std::cout << "Telemetry disabled. You can change this anytime with: settings telemetry enable\n";
    }
    std::cout << "\n";
}

void Engine::flush_telemetry_prompt() {
    if (!pending_telemetry_prompt_) return;
    pending_telemetry_prompt_ = false;
    if (!settings_.has_telemetry_preference()) {
        prompt_telemetry_consent();
    }
}

bool Engine::consume_quest_celebration() {
    bool pending = quest_completed_this_command_;
    quest_completed_this_command_ = false;
    return pending;
}

void Engine::play_quest_celebration() {
    if (!consume_quest_celebration()) return;
    smite::play_quest_confetti(std::cout);
}

// --------------------------
// REPL
// --------------------------
void Engine::repl() {
    // Load previous history from ~/.smite/.history
    std::string history_file = get_history_file_path();
    fs::create_directories(smite::paths::user_state_dir());
    read_history(history_file.c_str());

    // First launch starts the guided path. Telemetry is asked only after a quest
    // is completed, so a new player is not stopped before they have played.
    std::string intro = begin_first_quest_if_needed();
    if (!intro.empty()) {
        std::cout << intro;
    }

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
                play_quest_celebration();
                flush_telemetry_prompt();
                continue;
            }
        }

        // Normal dispatch
        std::string output = dispatch_command(cmd);
        if (!output.empty()) {
            std::cout << output;
        }
        std::cout << std::endl;
        play_quest_celebration();
        flush_telemetry_prompt();
    }

    // Save history to ~/.smite/.history (reuse variable from earlier)
    write_history(history_file.c_str());
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