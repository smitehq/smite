#include "globals.h"
#include "engine.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <yaml-cpp/yaml.h>  // For quests
#include "module_interface.h"
#include <unordered_map>
#include <functional>

namespace fs = std::filesystem;

Engine::Engine(const std::string& modulesDir) : modules_dir(modulesDir), router() {}

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
        // Plug-and-play stub: For static, assume main() or registry calls create_module_<name>()
        // E.g., if (mp ends with "/kubernetes") add_module(create_module_kubernetes());
        // Dynamic DLL: LoadLib(mp + "/module.dll"), GetProc("create_module"), etc.
        // For now: Manual in main.cpp
    }
    return true;
}

void Engine::add_module(std::shared_ptr<SmiteModule> module) {
    router.add_module(module);
    fmt::print("Added module: {} (supports: {} commands)\n", 
               module->name(), module->registered_prefixes().size());

    // Quest stub: Only load/eval active ones (default empty set)
    std::string mod_path = modules_dir + "/" + module->name();
    std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
    if (fs::exists(quests_file) && active_quests[module->name()].size() > 0) {
        try {
            YAML::Node quests_yaml = YAML::LoadFile(quests_file);
            if (quests_yaml["quests"]) {
                fmt::print("Quest progress for {}:\n", module->name());
                int id = 0;
                for (auto q : quests_yaml["quests"]) {
                    if (active_quests[module->name()].count(id) > 0) {  // Only active
                        bool passed = module->evaluate_condition(q["condition"]);
                        fmt::print("  {}: {} ({})\n", q["title"].as<std::string>(), 
                                   passed ? "Complete" : "Pending", q["description"].as<std::string>());
                    }
                    id++;
                }
            }
        } catch (const std::exception& ex) {
            fmt::print("Quest load warning: {}\n", ex.what());
        }
    } else {
        fmt::print("No active quests for {}; use 'quests {}' to select.\n", module->name(), module->name());
    }
}

// Helpers
static std::string trim(const std::string& str) {
    size_t l = str.find_first_not_of(" \t\r\n");
    if (l == std::string::npos) return "";
    size_t r = str.find_last_not_of(" \t\r\n");
    return str.substr(l, r - l + 1);
}

static void cout_flush(const std::string& msg) {
    std::cout << msg << std::flush;
}

// Extracted quests handler
void Engine::handle_quests(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        cout_flush("Available quests by module:\n");
        for (const auto& mod : router.get_modules()) {
            std::string mod_path = modules_dir + "/" + mod->name();
            std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
            if (fs::exists(quests_file)) {
                YAML::Node quests_yaml = YAML::LoadFile(quests_file);
                if (quests_yaml["quests"]) {
                    cout_flush(mod->name() + ":\n");
                    int id = 0;
                    for (auto q : quests_yaml["quests"]) {
                        bool active = active_quests[mod->name()].count(id) > 0;
                        cout_flush("  " + std::to_string(id) + ": " + q["title"].as<std::string>() + " [" + (active ? "ACTIVE" : "INACTIVE") + "]\n");
                        id++;
                    }
                }
            }
        }
        cout_flush("\nUsage: quests <module> [id] to list/activate (e.g., quests kubernetes, quests linux 0)\n");
        return;
    }
    auto mod_name = tokens[1];
    if (tokens.size() < 3) {
        // List for module
        std::string mod_path = modules_dir + "/" + mod_name;
        std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
        if (!fs::exists(quests_file)) {
            cout_flush("No quests.yaml for module '" + mod_name + "'.\n");
            return;
        }
        YAML::Node quests_yaml = YAML::LoadFile(quests_file);
        if (quests_yaml["quests"]) {
            cout_flush(mod_name + " quests:\n");
            int id = 0;
            for (auto q : quests_yaml["quests"]) {
                bool active = active_quests[mod_name].count(id) > 0;
                cout_flush("  " + std::to_string(id) + ": " + q["title"].as<std::string>() + " [" + (active ? "ACTIVE" : "INACTIVE") + "]\n");
                id++;
            }
        } else {
            cout_flush("No quests defined for " + mod_name + ".\n");
        }
        return;
    }
    // Activate
    int quest_id;
    try {
        quest_id = stoi(tokens[2]);
    } catch (...) {
        cout_flush("Invalid quest ID; must be integer.\n");
        return;
    }
    active_quests[mod_name].insert(quest_id);
    cout_flush("Activated quest " + std::to_string(quest_id) + " for " + mod_name + ". Re-run app to check progress.\n");
}

void Engine::repl() {
    cout_flush("Smite.sh engine — type 'help' for commands, 'quit' to exit\n");
    std::string line;
    while (true) {
        cout_flush("$ ");
        if (!std::getline(std::cin, line)) {
            if (std::cin.eof() || std::cin.fail()) {
                cout_flush("\nEOF detected; exiting REPL.\n");
                break;
            }
            std::cin.clear();
            continue;
        }
        std::string cmd = trim(line);  // Trim early
        if (cmd.empty()) continue;
        auto tokens = CommandRouter::tokenize(cmd);  // Tokenize
        if (tokens.empty()) continue;

        // Engine command map
        static const std::unordered_map<std::string, std::function<void(const std::vector<std::string>&)>> engine_cmds = {
            {"quit", [](const auto&) { throw std::runtime_error("quit"); }},  // Break loop
            {"exit", [](const auto&) { throw std::runtime_error("exit"); }},
            {"help", [this](const auto&) {
                auto cmds = router.list_commands();
                cout_flush("Registered command prefixes:\n");
                for (const auto& c : cmds) cout_flush("  " + c + "\n");
                cout_flush("Other engine commands: modules, quests, quit\n");
            }},
            {"modules", [this](const auto&) {
                cout_flush("Modules loaded:\n");
                for (const auto& mod : router.get_modules()) {
                    cout_flush("  - " + mod->name() + " (commands: " + std::to_string(mod->registered_prefixes().size()) + ")\n");
                }
                cout_flush("(Call 'help' to see all commands)\n");
            }},
            {"quests", [this](const auto& t) { handle_quests(t); }}
        };

        auto engine_it = engine_cmds.find(tokens[0]);
        if (engine_it != engine_cmds.end()) {
            try {
                engine_it->second(tokens);  // Dispatch
                if (tokens[0] == "quit" || tokens[0] == "exit") break;  // Handle throw
            } catch (const std::runtime_error&) {
                break;  // Quit/exit
            }
            continue;
        }

        // Route to modules
        std::string out = router.handle_input(cmd);
        if (out.empty()) {
            cout_flush("Unknown command.\n");
        } else {
            cout_flush(out);
        }
    }

    cout_flush("\nJourney ends. Farewell, Apprentice.\n");
}