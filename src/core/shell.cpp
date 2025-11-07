#include "../globals.h"
#include "utils.h"
#include "shell.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

// --- after ---
Shell::Shell() : root(std::make_unique<Dir>()), command_registry(), alias_registry() {}

std::string Shell::name() const {
    return "shell";
}

bool Shell::load_from_path(const std::string&) {
   // If load_from_path can be called multiple times, reset the root so we start clean.
    //root = std::make_unique<Dir>();

    // prepare base dirs and current/home
    home = std::string("/home/") + globals::PLAYER_NAME;
    current_dir = home;

    build_base_state();
    register_builtin_commands();

    return true;
}

void Shell::build_base_state() {
    // create base directories
    root->subdirs["home"] = make_unique<Dir>();
    root->subdirs["tmp"] = make_unique<Dir>();
    root->subdirs["etc"] = make_unique<Dir>();
    root->subdirs["bin"] = make_unique<Dir>();
    root->subdirs["var"] = make_unique<Dir>();

    // /home/player
    root->subdirs["home"]->subdirs[globals::PLAYER_NAME] = make_unique<Dir>();
    Dir* home_dir = root->subdirs["home"]->subdirs[globals::PLAYER_NAME].get();

    // prepopulate a few files
    home_dir->files["readme.txt"] = make_unique<File>(File{
        "Welcome to smiteOS!\nType 'ls' to see files.\n", "rw-r--r--"
    });

    // /etc/motd
    root->subdirs["etc"]->files["motd"] = make_unique<File>(File{
        "Welcome adventurer.\nYour mission begins here.\n", "rw-r--r--"
    });

    // /bin/help (for flavor)
    root->subdirs["bin"]->files["help"] = make_unique<File>(File{
        "Usage: help <command>\nCurrently supported: ls, cd, cat, echo, alias, unalias, pwd, touch.\n",
        "rwxr-xr-x"
    });

    std::cout << "Base shell filesystem initialized at " << current_dir << "\n";
}


// ---------------- Command Registration ----------------

void Shell::register_command(const std::string& name, std::function<std::string(const std::vector<std::string>&)> handler) {
    command_registry[name] = std::move(handler);
}

void Shell::register_builtin_commands() {
    register_command("ls", [this](const auto& args) -> std::string {
        std::string target;
        bool long_format = false;

        // parse args
        for (const auto& arg : args) {
            if (arg == "-l") long_format = true;
            else if (target.empty()) target = arg; // first non-flag arg is the path
        }
        if (target.empty()) target = current_dir; // default to current dir

        Dir* dir = get_dir(resolve_path(target));
        if (!dir) return "ls: No such directory: " + target + "\n";

        std::stringstream out;

        if (long_format) {
            // simple total (approximate)
            size_t total_blocks = 0;
            for (const auto& f : dir->files) total_blocks += f.second->content.size() / 512 + 1;
            out << "total " << total_blocks << "\n";

            for (const auto& d : dir->subdirs) {
                out << "drwxr-xr-x 1 " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " 0 Nov  1 00:00 " << d.first << "\n";
            }
            for (const auto& f : dir->files) {
                out << f.second->perms << " 1 " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " "
                    << f.second->content.size() << " Nov  1 00:00 "
                    << f.first << "\n";
            }
        } else {
            for (const auto& f : dir->files) out << f.first << " ";
            for (const auto& d : dir->subdirs) out << d.first << "/ ";
            out << "\n";
        }

        return out.str();
    });

    register_command("cd", [this](const auto& args) -> string {
        if (args.empty()) return "cd: No directory provided\n";
        string target = args[0];

        if (target == "..") {
            // Go up one directory
            if (current_dir == "/") return ""; // already root
            size_t pos = current_dir.find_last_of('/');
            if (pos == 0) current_dir = "/";
            else if (pos != string::npos) current_dir = current_dir.substr(0, pos);
            return "";
        }

        string resolved = resolve_path(target);
        Dir* dir = get_dir(resolved);
        if (!dir) return "cd: No such directory: " + target + "\n";
        current_dir = resolved;

        return "";
    });

    register_command("pwd", [this](const auto&) -> string {
        return current_dir + "\n";
    });

    register_command("cat", [this](const auto& args) -> string {
        if (args.empty()) return "cat: No file provided\n";
        string file_path = resolve_path(args[0]);

        auto [dir, filename] = get_dir_and_file(file_path);
        if (!dir || dir->files.count(filename) == 0)
            return "cat: " + file_path + ": No such file\n";

        return dir->files[filename]->content + "\n";
    });

    register_command("touch", [this](const auto& args) -> string {
        if (args.empty()) return "touch: No file provided\n";
        string file_path = resolve_path(args[0]);
        auto [dir, filename] = get_dir_and_file(file_path);

        if (!dir) return "touch: Invalid path\n";
        dir->files[filename] = make_unique<File>(File{"", "rw-r--r--"});
        return "";
    });

    register_command("echo", [this](const auto& args) {
        string out;
        for (const auto& a : args) out += a + " ";
        return out + "\n";
    });

    // register_command("alias", [this](const auto& args) -> std::string {
    //     if (args.empty()) {
    //         // List all aliases
    //         std::stringstream out;
    //         for (const auto& kv : alias_registry)
    //             out << kv.first << "='" << kv.second << "'\n";
    //         return out.str();
    //     }

    //     // Join everything back into one string so we can handle spaces in quotes properly
    //     std::string line;
    //     for (size_t i = 0; i < args.size(); ++i) {
    //         if (i) line += " ";
    //         line += args[i];
    //     }

    //     // Parse one or multiple alias definitions from the same command
    //     std::stringstream ss(line);
    //     std::string token;
    //     while (std::getline(ss, token, ' ')) {
    //         if (token.empty()) continue;

    //         size_t eq = token.find('=');
    //         if (eq == std::string::npos) continue;

    //         std::string name = token.substr(0, eq);
    //         std::string value = token.substr(eq + 1);

    //         // Handle quoted values like 'ls -l'
    //         if (!value.empty()) {
    //             if ((value.front() == '\'' && value.back() == '\'') ||
    //                 (value.front() == '"' && value.back() == '"')) {
    //                 value = value.substr(1, value.size() - 2);
    //             } else {
    //                 // Support multi-word alias definitions like ll='ls -l'
    //                 size_t next = line.find(value) + value.size();
    //                 if (next < line.size() && line[next] == ' ') {
    //                     value += line.substr(next, line.size() - next);
    //                     // Trim quotes if present at the end
    //                     if (!value.empty() && value.front() == '\'' && value.back() == '\'')
    //                         value = value.substr(1, value.size() - 2);
    //                 }
    //             }
    //         }

    //         alias_registry[name] = value;
    //     }

    //     return "";
    // });

    // register_command("unalias", [this](const auto& args) -> std::string {
    //     if (args.empty()) return "unalias: specify alias to remove\n";
    //     for (const auto& a : args) alias_registry.erase(a);
    //     return "";
    // });

}

// ---------------- Path Utilities ----------------

std::string Shell::expand_home(const std::string& path_arg) const {
    if (path_arg.empty()) return path_arg;
    if (path_arg[0] == '~') {
        if (path_arg.size() == 1 || path_arg[1] == '/')
            return home + path_arg.substr(1);
    }
    return path_arg;
}

std::string Shell::resolve_path(const std::string& path_arg) const {
    if (path_arg.empty()) return current_dir;

    std::string path = expand_home(path_arg);

    if (path[0] == '/') return path; // absolute
    if (current_dir == "/") return "/" + path;
    return current_dir + "/" + path;
}

pair<Dir*, string> Shell::get_dir_and_file(const std::string& full_path) const {
    string path = full_path;
    if (path.empty() || path == "/") return {root.get(), ""};

    size_t pos = path.find_last_of('/');
    string dir_part = (pos == string::npos) ? "" : path.substr(0, pos);
    string filename = (pos == string::npos) ? path : path.substr(pos + 1);

    Dir* dir = get_dir(dir_part.empty() ? current_dir : dir_part);
    return {dir, filename};
}

Dir* Shell::get_dir(const string& path_arg) const {
    if (path_arg.empty() || path_arg == "/") return root.get();

    vector<string> parts;
    stringstream ss(path_arg[0] == '/' ? path_arg.substr(1) : path_arg);
    string token;
    while (getline(ss, token, '/')) {
        if (token.empty() || token == ".") continue;
        parts.push_back(token);
    }

    Dir* current = root.get();
    for (const auto& part : parts) {
        if (current->subdirs.count(part) == 0) return nullptr;
        current = current->subdirs.at(part).get();
    }

    return current;
}

// ---------------- Module Interface ----------------

std::string Shell::run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) {
    std::string actualCmd = cmdPrefix;
    std::vector<std::string> actualArgs = args;

    // Check if the command is a dynamic alias
    // auto aliasIt = alias_registry.find(cmdPrefix);
    // if (aliasIt != alias_registry.end()) {
    //     auto aliasTokens = Utils::tokenize_command(aliasIt->second);
    //     if (!aliasTokens.empty()) {
    //         actualCmd = aliasTokens[0];
    //         actualArgs = std::vector<std::string>(aliasTokens.begin() + 1, aliasTokens.end());
    //         actualArgs.insert(actualArgs.end(), args.begin(), args.end()); // append original args
    //     }
    // }

    auto it = command_registry.find(actualCmd);
    if (it == command_registry.end())
        return "Command not found: " + actualCmd + "\n";
    return it->second(actualArgs);
}


bool Shell::evaluate_condition(const YAML::Node&) { return false; }

std::vector<std::string> Shell::registered_prefixes() const {
    std::vector<std::string> out;
    for (const auto& kv : command_registry) out.push_back(kv.first);
    return out;
}

bool Shell::supports_command(const std::string& cmdPrefix) const {
    return command_registry.find(cmdPrefix) != command_registry.end();
}

// ---------------- Factory ----------------

std::shared_ptr<SmiteModule> create_module_shell() {
    return std::make_shared<Shell>();
}
