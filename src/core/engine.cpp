#include "utils.h"
#include "engine.h"
#include <filesystem>
#include <iostream>
#include <yaml-cpp/yaml.h>
#include <sstream>
#include <readline/readline.h>
#include <readline/history.h>

namespace fs = std::filesystem;

// --------------------------
// Engine Constructor
// --------------------------
Engine::Engine(const std::string& modulesDir) : modules_dir(modulesDir), router(), active_quests() {
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
    register_command("quests", [this](const auto& t) -> std::string {
        std::ostringstream oss;
        if (t.size() < 2) {
            oss << "Available quests by module:\n";
            for (const auto& mod : router.get_modules()) {
                oss << mod->name() << ":\n";
                oss << list_quests_for_module(mod->name());
            }
        } else {
            auto mod_name = t[1];
            if (t.size() < 3) {
                oss << list_quests_for_module(mod_name);
            } else {
                int quest_id;
                try { quest_id = std::stoi(t[2]); } 
                catch (...) { return "Invalid quest ID; must be integer.\n"; }
                active_quests[mod_name].insert(quest_id);
                oss << "Activated quest " << quest_id << " for " << mod_name << ". Re-run app to check progress.\n";
            }
        }
        return oss.str();
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
    std::cout << "Added module: " << module->name() 
              << " (supports " << module->registered_prefixes().size() << " commands)\n";
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
    std::cout << "Smite.sh engine — type 'help' for commands, 'quit' to exit\n";

    // Load previous history
    read_history(".smite_history");

    // setup for auto-completion
    router.setup_readline_completion();

    while (true) {
        char* input = readline("$ ");
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

// --------------------------
// Quests
// --------------------------
std::string Engine::list_quests_for_module(const std::string& mod_name) const {
    std::ostringstream oss;
    std::string mod_path = modules_dir + "/" + mod_name;
    std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
    if (!fs::exists(quests_file)) {
        oss << "No quests.yaml for module '" << mod_name << "'.\n";
        return oss.str();
    }
    YAML::Node quests_yaml = YAML::LoadFile(quests_file);
    if (!quests_yaml["quests"]) return "No quests defined for " + mod_name + ".\n";

    oss << mod_name << " quests:\n";
    int id = 0;
    for (const auto& q : quests_yaml["quests"]) {
        bool active = active_quests.count(mod_name) > 0 && active_quests.at(mod_name).count(id) > 0;
        oss << "  " << id << ": " << q["title"].as<std::string>() << " [" 
            << (active ? "ACTIVE" : "INACTIVE") << "]\n";
        ++id;
    }
    return oss.str();
}

// --------------------------
// Handle quests command
// --------------------------
void Engine::handle_quests(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        cout_flush("Available quests by module:\n");
        for (auto& mod : router.get_modules()) {
            cout_flush(mod->name() + ":\n");
            cout_flush(list_quests_for_module(mod->name()));
        }
        cout_flush("\nUsage: quests <module> [id] to list/activate\n");
        return;
    }
    std::string mod_name = tokens[1];
    if (tokens.size() < 3) {
        cout_flush(list_quests_for_module(mod_name));
        return;
    }
    try {
        int quest_id = std::stoi(tokens[2]);
        active_quests[mod_name].insert(quest_id);
        cout_flush("Activated quest " + std::to_string(quest_id) + " for " + mod_name + ". Re-run app to check progress.\n");
    } catch (...) {
        cout_flush("Invalid quest ID; must be integer.\n");
    }
}
