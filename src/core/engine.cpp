#include "globals.h"
#include "utils.h"
#include "engine.h"
#include "shell.h"
#include <filesystem>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>
#include "quest.h"

namespace fs = std::filesystem;

// --------------------------
// Engine Constructor
// --------------------------
Engine::Engine(const std::string& modulesDir) : modules_dir(modulesDir), router(), quests(modulesDir) {
    quests.load_all_quests();

    // Register default engine commands
    register_command("quit", [](const auto&) -> std::string { throw std::runtime_error("quit"); });
    register_command("exit", [](const auto&) -> std::string { throw std::runtime_error("quit"); });

    register_command("help", [this](const auto&) -> std::string {
        std::ostringstream oss;
        oss << "Registered command prefixes:\n";
        for (const auto& c : router.list_commands()) oss << "  " << c << "\n";
        oss << "Other engine commands: modules, quests, quit\n";
        return oss.str();
    });

    register_command("modules", [this](const auto&) -> std::string {
        std::ostringstream oss;
        oss << "Modules loaded:\n";
        for (const auto& mod : router.get_modules()) {
            oss << "  - " << mod->name() << " (commands: " << mod->registered_prefixes().size() << ")\n";
        }
        return oss.str();
    });

    register_command("quests", [this](const auto& args) -> std::string {
        if (args.size() < 2) {
            std::ostringstream oss;
            for (const auto& mod : router.get_modules()) {
                if (quests.get_quests_for_module(mod->name()).empty()) continue;
                oss << mod->name() << ":\n" << quests.list_quests(mod->name());
            }
            return oss.str();
        }

        const auto& mod = args[1];
        if (args.size() == 2) return quests.list_quests(mod);
        
        const auto& quest_id = args[2];
        if (!quests.activate_quest(mod, quest_id)) return "Quest not found.\n";

        auto mod_ptr = router.get_module_by_name(mod);
        if (!mod_ptr) {
            std::cout << "Module not found: " << mod << "\n";
            return "\n";
        }
        mod_ptr->activate_quest(quest_id);


        //router.get_module_by_name(mod)->activate_quest(quest_id);
        return fmt::format("Activated quest {} for {}.\n", quest_id, mod);
    });
}

// --------------------------
// Module management
// --------------------------
std::vector<std::string> Engine::discover_module_paths() const {
    std::vector<std::string> out;
    for (auto &p : fs::directory_iterator(modules_dir)) {
        if (!p.is_directory()) continue;
        out.push_back(p.path().string());
    }
    return out;
}

bool Engine::load_modules() {
    auto module_paths = discover_module_paths();
    for (auto &mp : module_paths) {
        // Static factory approach (manual in main.cpp for now)
    }
    return true;
}

void Engine::add_module(std::shared_ptr<SmiteModule> module) {
    router.add_module(module);
    // std::cout << "Added module: " << module->name() 
    //          << " (supports " << module->registered_prefixes().size() << " commands)\n";
}

// --------------------------
// Engine commands registry
// --------------------------
void Engine::register_command(const std::string& prefix, std::function<std::string(const std::vector<std::string>&)> handler) {
    engine_commands[prefix] = handler;
}

// --------------------------
// Command dispatch
// --------------------------
std::string Engine::dispatch_command(const std::string& cmd) {
    auto tokens = Utils::tokenize_command(cmd);
    if (tokens.empty()) return "";

    // Check engine commands first
    auto it = engine_commands.find(tokens[0]);
    if (it != engine_commands.end()) {
        return it->second(tokens);
    }

    // Otherwise, route to modules
    return router.handle_input(cmd);
}

// --------------------------
// REPL
// --------------------------
void Engine::repl() {
    // Load previous history
    read_history(".smite_history");

    // setup for auto-completion
    auto shell = std::dynamic_pointer_cast<Shell>(get_module_by_name("shell"));
    shell->setup_readline_completion(&router);

    while (true) {
        char* input = readline(get_prompt().c_str());
        if (!input) break;  // Ctrl+D
        std::string cmd = trim(input);
        if (!cmd.empty()) add_history(input);
        free(input);

        if (cmd.empty()) continue;

        // Handle multi-command chaining "&&"
        size_t and_pos = cmd.find(" && ");
        if (and_pos != std::string::npos) {
            std::string cmd1 = trim(cmd.substr(0, and_pos));
            std::string cmd2 = trim(cmd.substr(and_pos + 4));
            if (!cmd1.empty() && !cmd2.empty()) {
                try {
                    std::string out1 = dispatch_command(cmd1);
                    if (!out1.empty()) std::cout << out1;
                    if (out1.find("Error") == std::string::npos &&
                        out1.find("Unknown") == std::string::npos) {
                        std::string out2 = dispatch_command(cmd2);
                        if (!out2.empty()) std::cout << out2;
                    } else {
                        std::cout << " (Chain stopped on error)\n";
                    }
                } catch (const std::runtime_error&) {
                    std::cout << "\nExiting REPL.\n";
                    break;
                }
                continue;
            }
        }

        // Normal dispatch
        try {
            std::string out = dispatch_command(cmd);
            if (!out.empty()) std::cout << out;
        } catch (const std::runtime_error&) {
            std::cout << "\nExiting REPL.\n";
            break;
        }

        std::cout << std::endl;
    }

    // Save history
    write_history(".smite_history");
}

// --------------------------
// Helpers
// --------------------------
std::string Engine::trim(const std::string& str) {
    size_t l = str.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    size_t r = str.find_last_not_of(" \t\r\n");
    return str.substr(l, r - l + 1);
}

void Engine::cout_flush(const std::string& msg) {
    std::cout << msg << std::flush;
}

std::string Engine::get_prompt() {
    auto shell = std::dynamic_pointer_cast<Shell>(get_module_by_name("shell"));
    std::string user = globals::PLAYER_NAME;
    std::string host = globals::HOSTNAME;
    std::string dir = shell ? shell->get_current_dir() : "~";
    if (dir == shell->get_home()) dir = "~";
    return fmt::format("\x1b[1;32m{}@{}\x1b[0m:\x1b[34m{}\x1b[0m$ ", user, host, dir);
}

// expose means for module cross talk
std::shared_ptr<SmiteModule> Engine::get_module_by_name(const std::string& name) const {
    for (auto& mod : router.get_modules()) {
        if (mod->name() == name) return mod;
    }
    return nullptr;
}