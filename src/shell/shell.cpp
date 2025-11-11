#include "shell.h"
#include "../globals.h"
#include "../core/utils.h"

// Include all command headers
#include "commands/ls.h"
#include "commands/cd.h"
#include "commands/pwd.h"
#include "commands/cat.h"
#include "commands/echo.h"
#include "commands/touch.h"
#include "commands/chmod.h"
#include "commands/nano.h"

#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <readline/readline.h>
#include <cstdlib>

using namespace std;
namespace fs = std::filesystem;

// Thread-local context for auto-completion
thread_local Shell::CompletionContext* g_completion_context = nullptr;

Shell::Shell() : root_(std::make_unique<Dir>()) {}

std::string Shell::name() const {
    return "shell";
}

bool Shell::load_from_path(const std::string&) {
    home_ = std::string("/home/") + globals::PLAYER_NAME;
    current_dir_ = home_;

    build_base_state();
    register_all_commands();

    return true;
}

void Shell::build_base_state() {
    // Create base directories
    root_->subdirs["home"] = make_unique<Dir>();
    root_->subdirs["tmp"] = make_unique<Dir>();
    root_->subdirs["etc"] = make_unique<Dir>();
    root_->subdirs["bin"] = make_unique<Dir>();
    root_->subdirs["var"] = make_unique<Dir>();

    // /home/player
    root_->subdirs["home"]->subdirs[globals::PLAYER_NAME] = make_unique<Dir>();
    Dir* home_dir = root_->subdirs["home"]->subdirs[globals::PLAYER_NAME].get();

    home_dir->files["readme.txt"] = make_unique<File>(File{
        "Welcome to smiteOS!\nType 'ls' to see files.\n"
    });

    root_->subdirs["etc"]->files["motd"] = make_unique<File>(File{
        "Welcome adventurer.\nYour mission begins here.\n"
    });

    root_->subdirs["bin"]->files["help"] = make_unique<File>(File{
        "Usage: help <command>\nCurrently supported: ls, cd, cat, echo, pwd, touch.\n"
    });
}

// ============================================
// Command Registration
// ============================================

void Shell::register_all_commands() {
    using namespace shell_commands;

    // Template register_command automatically passes 'this' to command factories
    register_command("ls",    cmd_ls);
    register_command("cd",    cmd_cd);
    register_command("pwd",   cmd_pwd);
    register_command("cat",   cmd_cat);
    register_command("echo",  cmd_echo);
    register_command("touch", cmd_touch);
    register_command("chmod", cmd_chmod);
    register_command("nano",  cmd_nano);
}

// ============================================
// Path Utilities
// ============================================

std::string Shell::expand_home(const std::string& path_arg) const {
    if (path_arg.empty()) return path_arg;
    if (path_arg[0] == '~') {
        if (path_arg.size() == 1 || path_arg[1] == '/') {
            return home_ + path_arg.substr(1);
        }
    }
    return path_arg;
}

std::string Shell::resolve_path(const std::string& path_arg) const {
    if (path_arg.empty()) return current_dir_;
    std::string path = expand_home(path_arg);
    if (!path.empty() && path[0] == '/') return path;
    if (current_dir_ == "/") return "/" + path;
    return current_dir_ + "/" + path;
}

pair<Dir*, string> Shell::get_dir_and_file(const std::string& full_path) const {
    string path = full_path;
    if (path.empty() || path == "/") return {root_.get(), ""};

    size_t pos = path.find_last_of('/');
    string dir_part = (pos == string::npos) ? "" : path.substr(0, pos);
    string filename = (pos == string::npos) ? path : path.substr(pos + 1);

    Dir* dir = get_dir(dir_part.empty() ? current_dir_ : dir_part);
    return {dir, filename};
}

std::optional<Dir*> Shell::get_dir_safe(const std::string& path_arg) const {
    if (path_arg.empty() || path_arg == "/") return root_.get();
    if (path_arg.find("..") != std::string::npos) return std::nullopt;

    vector<string> parts;
    stringstream ss(path_arg[0] == '/' ? path_arg.substr(1) : path_arg);
    string token;
    
    while (getline(ss, token, '/')) {
        if (token.empty() || token == ".") continue;
        if (token == "..") return std::nullopt;
        parts.push_back(token);
    }

    Dir* current = root_.get();
    for (const auto& part : parts) {
        auto it = current->subdirs.find(part);
        if (it == current->subdirs.end()) return std::nullopt;
        current = it->second.get();
    }
    return current;
}

Dir* Shell::get_dir(const string& path_arg) const {
    auto result = get_dir_safe(path_arg);
    return result.value_or(nullptr);
}

// ============================================
// Module Interface
// ============================================

std::string Shell::run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) {
    auto it = command_registry_.find(cmdPrefix);
    if (it == command_registry_.end()) {
        return "Command not found: " + cmdPrefix + "\n";
    }

    try {
        return it->second(args);  // Lambda already captures context
    } catch (const std::exception& e) {
        return std::string("Error executing command: ") + e.what() + "\n";
    }
}

bool Shell::evaluate_condition(const YAML::Node&) { 
    return false; 
}

std::vector<std::string> Shell::registered_prefixes() const {
    std::vector<std::string> out;
    out.reserve(command_registry_.size());
    
    for (const auto& [cmd_name, _] : command_registry_) {
        out.push_back(cmd_name);
    }
    return out;
}

bool Shell::supports_command(const std::string& cmdPrefix) const {
    return command_registry_.find(cmdPrefix) != command_registry_.end();
}

// ============================================
// Autocompletion (unchanged)
// ============================================

static char* universal_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t index;

    if (state == 0) {
        matches.clear();
        index = 0;

        if (!g_completion_context || !g_completion_context->router || !g_completion_context->shell) {
            return nullptr;
        }

        std::string line(rl_line_buffer);
        auto tokens = Utils::tokenize_command(line);
        bool after_space = !line.empty() && isspace(line.back());

        size_t token_index;
        std::string current_token;
        
        if (tokens.empty() || after_space) {
            token_index = tokens.size();
            current_token = "";
        } else {
            token_index = tokens.size() - 1;
            current_token = tokens.back();
        }

        auto cmd_matches = g_completion_context->router->complete_command(
            tokens, token_index, current_token);
        matches.insert(matches.end(), cmd_matches.begin(), cmd_matches.end());

        if (matches.empty() && token_index > 0) {
            auto dir_opt = g_completion_context->shell->get_dir_safe(
                g_completion_context->shell->get_current_dir());
            
            if (dir_opt.has_value()) {
                Dir* dir = dir_opt.value();
                
                for (const auto& [name, file] : dir->files) {
                    if (name.size() >= current_token.size() &&
                        name.compare(0, current_token.size(), current_token) == 0) {
                        matches.push_back(name);
                    }
                }
                
                for (const auto& [name, subdir] : dir->subdirs) {
                    if (name.size() >= current_token.size() &&
                        name.compare(0, current_token.size(), current_token) == 0) {
                        matches.push_back(name + "/");
                    }
                }
            }
        }

        std::sort(matches.begin(), matches.end());
        matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    }

    if (index < matches.size()) {
        return strdup(matches[index++].c_str());
    }

    return nullptr;
}

static char** shell_completion(const char* text, int start, int end) {
    (void)start;
    (void)end;
    rl_attempted_completion_over = 1;
    return rl_completion_matches(text, universal_generator);
}

void Shell::setup_readline_completion(Router* router) {
    if (!router) {
        std::cerr << "Warning: Cannot setup completion with null router\n";
        return;
    }

    static CompletionContext context;
    context.router = router;
    context.shell = this;
    g_completion_context = &context;

    rl_attempted_completion_function = shell_completion;
    rl_completion_append_character = ' ';
    rl_basic_word_break_characters = " \t\n";
    rl_bind_key('\t', rl_complete);
}

// ============================================
// Factory Function
// ============================================

std::shared_ptr<SmiteModule> create_module_shell() {
    return std::make_shared<Shell>();
}