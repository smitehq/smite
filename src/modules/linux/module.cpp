#include "globals.h"
#include "module.h"
#include "core/module_interface.h"
#include <yaml-cpp/yaml.h>
#include <memory>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <algorithm>
#include <functional>
#include <unordered_map>

using namespace std;
namespace fs = std::filesystem;
using YAML::Node;

string LinuxModule::name() const {
    return "linux";
}

bool LinuxModule::load_from_path(const std::string& modulePath) {
    path = modulePath;
    try {
        Node state_node = YAML::LoadFile((fs::path(modulePath) / "state.yaml").string());
        string scenario = state_node["scenario"].as<string>("default");
        Node scenario_fs = state_node["scenarios"][scenario];
        if (scenario_fs["filesystem"]) {
            Node yaml_fs = scenario_fs["filesystem"];
            function<void(const Node&, Dir&)> parse_dir = [&](const Node& node, Dir& current_dir) {
                for (const auto& entry : node) {
                    string key = entry.first.as<string>();
                    if (key.empty()) continue;
                    Node value = entry.second;
                    if (value.IsMap()) {
                        if (value["content"].IsDefined()) {
                            // File node
                            string content = value["content"].as<string>("");
                            string perms = value["perms"].as<string>("rw-r--r--");
                            current_dir.files[key] = make_unique<File>(File{content, perms});
                        } else {
                            // Directory node
                            current_dir.subdirs[key] = make_unique<Dir>();
                            parse_dir(value, *current_dir.subdirs[key]);
                        }
                    } else {
                        // Scalar file
                        string content = value.as<string>("");
                        current_dir.files[key] = make_unique<File>(File{content, "rw-r--r--"});
                    }
                }
            };

            root = make_unique<Dir>();
            parse_dir(yaml_fs, *root);
            cout << "Loaded Linux FS state: scenario '" << scenario << "' (root loaded)\n";
        }
        if (scenario_fs["current_dir"]) {
            current_dir = scenario_fs["current_dir"].as<std::string>(std::string("/home/") + globals::PLAYER_NAME);
            home = current_dir;  // for ~ expansion
        }
    } catch (const std::exception &ex) {
        cout << "LinuxModule load error: " << ex.what() << "\n";
        return false;
    }

    // Register built-in shell commands
    register_builtin_commands();

    cout << "Linux shell ready (commands: " << command_registry.size() << ")\n";
    return true;
}

void LinuxModule::register_command(const std::string& name, std::function<std::string(const std::vector<std::string>&)> handler) {
    command_registry[name] = std::move(handler);
}

void LinuxModule::register_builtin_commands() {
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
                out << f.second->perms << " 1 "<< globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " "
                    << f.second->content.size() << " Nov  1 00:00 "
                    << f.first << "\n";
            }
        } else {
            for (const auto& f : dir->files) out << f.first << " ";
            for (const auto& d : dir->subdirs) out << d.first << "/ ";
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
}

std::string LinuxModule::run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) {
    auto it = command_registry.find(cmdPrefix);
    if (it == command_registry.end())
        return "Command not found: " + cmdPrefix + "\n";

    return it->second(args);
}

std::string LinuxModule::resolve_path(const std::string& path_arg) const {
    if (path_arg.empty()) return current_dir;
    if (path_arg[0] == '/') return path_arg; // already absolute

    // Relative path
    if (current_dir == "/") return "/" + path_arg;
    return current_dir + "/" + path_arg;
}

pair<Dir*, string> LinuxModule::get_dir_and_file(const std::string& full_path) const {
    string path = full_path;
    if (path.empty() || path == "/") return {root.get(), ""};

    size_t pos = path.find_last_of('/');
    string dir_part = (pos == string::npos) ? "" : path.substr(0, pos);
    string filename = (pos == string::npos) ? path : path.substr(pos + 1);

    Dir* dir = get_dir(dir_part.empty() ? current_dir : dir_part);
    return {dir, filename};
}

Dir* LinuxModule::get_dir(const string& path_arg) const {
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
        if (part == "..") {
            // Can't easily move up in pointer traversal — stop here
            break;
        }
        if (current->subdirs.count(part) == 0) return nullptr;
        current = current->subdirs.at(part).get();
    }
    return current;
}

bool LinuxModule::evaluate_condition(const YAML::Node&) {
    // Future: implement conditions like file existence, perms, etc.
    return false;
}

std::vector<std::string> LinuxModule::registered_prefixes() const {
    std::vector<std::string> out;
    for (const auto& kv : command_registry)
        out.push_back(kv.first);
    return out;
}

bool LinuxModule::supports_command(const std::string& cmdPrefix) const {
    return command_registry.find(cmdPrefix) != command_registry.end();
}

std::size_t LinuxModule::fs_size() const {
    size_t count = 0;
    function<void(const Dir&)> count_entries = [&](const Dir& dir) {
        count += dir.files.size();
        for (const auto& subdir_pair : dir.subdirs) {
            count_entries(*subdir_pair.second);
        }
    };
    count_entries(*root);
    return count;
}

std::string LinuxModule::fs_debug() const {
    return "FS ready at " + current_dir + " (root dirs: " + to_string(root->subdirs.size()) + ")";
}

// Factory
std::shared_ptr<SmiteModule> create_module_linux() {
    return std::make_shared<LinuxModule>();
}
