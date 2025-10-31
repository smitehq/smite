#include "globals.h"
#include "engine.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <yaml-cpp/yaml.h>  // For quests
#include "module_interface.h"

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

void Engine::repl() {
    std::cout << "Smite.sh engine — type 'help' for commands, 'quit' to exit\n" << std::flush;
    std::string line;
    while (true) {
        std::cout << "$ " << std::flush;
        if (!std::getline(std::cin, line)) {
            if (std::cin.eof() || std::cin.fail()) {
                std::cout << "\nEOF detected; exiting REPL.\n" << std::flush;
                break;
            }
            std::cin.clear();
            continue;
        }
        // Basic trimming
        size_t l = line.find_first_not_of(" \t\r\n");
        if (l == std::string::npos) continue;
        size_t r = line.find_last_not_of(" \t\r\n");
        std::string cmd = line.substr(l, r - l + 1);
        auto tokens = CommandRouter::tokenize(cmd);  // Tokenize early
        if (tokens.empty()) continue;

        if (tokens[0] == "quit" || tokens[0] == "exit") break;  // Handle quit with args
        if (tokens[0] == "help") {
            auto cmds = router.list_commands();
            std::cout << "Registered command prefixes:\n";
            for (auto &c : cmds) std::cout << "  " << c << "\n";
            std::cout << "Other engine commands: modules, quests, quit\n" << std::flush;
            continue;
        }
        if (tokens[0] == "modules") {
            std::cout << "Modules loaded: (call 'help' to see commands)\n" << std::flush;
            continue;
        }
        if (tokens[0] == "quests") {
            if (tokens.size() < 2) {
                std::cout << "Available quests by module:\n";
                for (const auto& mod : router.get_modules()) {
                    std::string mod_path = modules_dir + "/" + mod->name();
                    std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
                    if (fs::exists(quests_file)) {
                        YAML::Node quests_yaml = YAML::LoadFile(quests_file);
                        if (quests_yaml["quests"]) {
                            std::cout << mod->name() << ":\n";
                            int id = 0;
                            for (auto q : quests_yaml["quests"]) {
                                bool active = active_quests[mod->name()].count(id) > 0;
                                std::cout << "  " << id << ": " << q["title"].as<std::string>() << " [" << (active ? "ACTIVE" : "INACTIVE") << "]\n";
                                id++;
                            }
                        }
                    }
                }
                std::cout << "\nUsage: quests <module> [id] to list/activate (e.g., quests kubernetes, quests linux 0)\n" << std::flush;
                continue;
            }
            auto mod_name = tokens[1];  // Module name
            // List for module if no ID
            if (tokens.size() < 3) {
                std::string mod_path = modules_dir + "/" + mod_name;
                std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
                if (!fs::exists(quests_file)) {
                    std::cout << "No quests.yaml for module '" << mod_name << "'.\n" << std::flush;
                    continue;
                }
                YAML::Node quests_yaml = YAML::LoadFile(quests_file);
                if (quests_yaml["quests"]) {
                    std::cout << mod_name << " quests:\n";
                    int id = 0;
                    for (auto q : quests_yaml["quests"]) {
                        bool active = active_quests[mod_name].count(id) > 0;
                        std::cout << "  " << id << ": " << q["title"].as<std::string>() << " [" << (active ? "ACTIVE" : "INACTIVE") << "]\n";
                        id++;
                    }
                } else {
                    std::cout << "No quests defined for " << mod_name << ".\n" << std::flush;
                }
                continue;
            }
            // Activate: quests <module> <id>
            int quest_id;
            try {
                quest_id = stoi(tokens[2]);
            } catch (...) {
                std::cout << "Invalid quest ID; must be integer.\n" << std::flush;
                continue;
            }
            active_quests[mod_name].insert(quest_id);
            std::cout << "Activated quest " << quest_id << " for " << mod_name << ". Re-run app to check progress.\n" << std::flush;
            continue;
        }
        // Route to modules (non-engine cmds)
        std::string out = router.handle_input(cmd);
        if (out.empty()) {
            std::cout << "Unknown command.\n" << std::flush;
        } else {
            std::cout << out << std::flush;
        }
    }
    std::cout << "\nJourney ends. Farewell, Apprentice.\n" << std::flush;
}