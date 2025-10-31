#include "globals.h"
#include "engine.h"
#include <filesystem>
#include <iostream>
#include <memory>
#include <yaml-cpp/yaml.h>  // For quests
#include "module_interface.h"
#include <functional>
#include <sstream>

namespace fs = std::filesystem;

Engine::Engine(const std::string& modulesDir) : modules_dir(modulesDir), router(), active_quests() {}

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
                for (const auto& q : quests_yaml["quests"]) {
                    if (active_quests[module->name()].count(id) > 0) {  // Only active
                        bool passed = module->evaluate_condition(q["condition"]);
                        fmt::print("  {}: {} ({})\n", q["title"].as<std::string>(), 
                                   passed ? "Complete" : "Pending", q["description"].as<std::string>());
                    }
                    ++id;
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

// List quests for a module
std::string Engine::list_quests_for_module(const std::string& mod_name) const {
    std::ostringstream oss;
    std::string mod_path = modules_dir + "/" + mod_name;
    std::string quests_file = (fs::path(mod_path) / "quests.yaml").string();
    if (!fs::exists(quests_file)) {
        oss << "No quests.yaml for module '" << mod_name << "'.\n";
        return oss.str();
    }
    YAML::Node quests_yaml = YAML::LoadFile(quests_file);
    if (quests_yaml["quests"]) {
        oss << mod_name << " quests:\n";
        int id = 0;
        for (const auto& q : quests_yaml["quests"]) {
            bool active = active_quests.at(mod_name).count(id) > 0;
            oss << "  " << id << ": " << q["title"].as<std::string>() << " [" << (active ? "ACTIVE" : "INACTIVE") << "]\n";
            ++id;
        }
    } else {
        oss << "No quests defined for " << mod_name << ".\n";
    }
    return oss.str();
}

// Extracted quests handler (Uses list_quests_for_module)
void Engine::handle_quests(const std::vector<std::string>& tokens) {
    if (tokens.size() < 2) {
        cout_flush("Available quests by module:\n");
        for (const auto& mod : router.get_modules()) {
            cout_flush(mod->name() + ":\n");
            cout_flush(list_quests_for_module(mod->name()));
        }
        cout_flush("\nUsage: quests <module> [id] to list/activate (e.g., quests kubernetes, quests linux 0)\n");
        return;
    }
    auto mod_name = tokens[1];
    if (tokens.size() < 3) {
        // List for module
        cout_flush(list_quests_for_module(mod_name));
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
        std::string cmd = trim(line);
        if (cmd.empty()) continue;

        // Check for "&&" chaining FIRST
        size_t and_pos = cmd.find(" && ");
        if (and_pos != std::string::npos) {
            std::string cmd1 = trim(cmd.substr(0, and_pos));
            std::string cmd2 = trim(cmd.substr(and_pos + 4));
            if (!cmd1.empty() && !cmd2.empty()) {
                // Run cmd1
                std::string out1 = dispatch_command(cmd1);
                if (!out1.empty()) cout_flush(out1);
                // Check for error
                if (out1.find("Error") != std::string::npos || out1.find("Unknown") != std::string::npos || out1.find("NotFound") != std::string::npos || out1.find("Invalid") != std::string::npos) {
                    cout_flush(" (Chain stopped on error)\n");
                } else {
                    // Run cmd2 if cmd1 succeeded
                    std::string out2 = dispatch_command(cmd2);
                    if (!out2.empty()) cout_flush(out2);
                }
                continue;
            }
        }

        // Normal dispatch
        std::string out = dispatch_command(cmd);
        if (!out.empty()) cout_flush(out);
    }
    cout_flush("\nJourney ends. Farewell, Apprentice.\n");
}

// Dispatch helper (map style for engine cmds or router)
std::string Engine::dispatch_command(const std::string& cmd) {
    auto tokens = CommandRouter::tokenize(cmd);
    if (tokens.empty()) return "";

    // Engine command map (return string for chaining)
    const std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> engine_cmds = {
        {"quit", [](const auto&) -> std::string { throw std::runtime_error("quit"); }},
        {"exit", [](const auto&) -> std::string { throw std::runtime_error("exit"); }},
        {"help", [this](const auto&) -> std::string {
            auto cmds = router.list_commands();
            std::ostringstream oss;
            oss << "Registered command prefixes:\n";
            for (const auto& c : cmds) oss << "  " << c << "\n";
            oss << "Other engine commands: modules, quests, quit\n";
            return oss.str();
        }},
        {"modules", [this](const auto&) -> std::string {
            std::ostringstream oss;
            oss << "Modules loaded:\n";
            for (const auto& mod : router.get_modules()) {
                oss << "  - " << mod->name() << " (commands: " << mod->registered_prefixes().size() << ")\n";
            }
            oss << "(Call 'help' to see all commands)\n";
            return oss.str();
        }},
        {"quests", [this](const auto& t) -> std::string {
            std::ostringstream oss;
            if (t.size() < 2) {
                oss << "Available quests by module:\n";
                for (const auto& mod : router.get_modules()) {
                    oss << mod->name() << ":\n";
                    oss << list_quests_for_module(mod->name());
                }
                oss << "\nUsage: quests <module> [id] to list/activate (e.g., quests kubernetes, quests linux 0)\n";
            } else {
                auto mod_name = t[1];
                if (t.size() < 3) {
                    oss << list_quests_for_module(mod_name);
                } else {
                    // Activate
                    int quest_id;
                    try {
                        quest_id = stoi(t[2]);
                    } catch (...) {
                        oss << "Invalid quest ID; must be integer.\n";
                        return oss.str();
                    }
                    active_quests[mod_name].insert(quest_id);
                    oss << "Activated quest " << quest_id << " for " << mod_name << ". Re-run app to check progress.\n";
                }
            }
            return oss.str();
        }}
    };

    auto engine_it = engine_cmds.find(tokens[0]);
    if (engine_it != engine_cmds.end()) {
        try {
            return engine_it->second(tokens);
        } catch (const std::runtime_error&) {
            throw;  // Re-throw for repl
        }
    }

    // Route to modules
    std::string out = router.handle_input(cmd);
    return out.empty() ? "Unknown command.\n" : out;
}