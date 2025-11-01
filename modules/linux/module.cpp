#include "module.h"
#include "core/module_interface.h"
#include <yaml-cpp/yaml.h>
#include <memory>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <map>
#include <algorithm>
#include <functional>  // For std::function in FileConfig


using namespace std;
namespace fs = std::filesystem;
using YAML::Node;

// Emulated FS: dir -> {file: (content, perms)}
using FS = map<string, map<string, pair<string, string>>>;

class LinuxModule : public SmiteModule {
public:
    LinuxModule() = default;
    ~LinuxModule() override = default;

    string name() const override { return "linux"; }

    bool load_from_path(const std::string& modulePath) override {
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
                            // Check if leaf file map (has "content" key) or dir
                            if (value["content"].IsDefined()) {
                                // File: Create file
                                string content = value["content"].as<string>("");
                                string perms = value["perms"].as<string>("rw-r--r--");
                                current_dir.files[key] = make_unique<File>();
                                current_dir.files[key]->content = content;
                                current_dir.files[key]->perms = perms;
                            } else {
                                // Dir: Create subdir, recurse
                                current_dir.subdirs[key] = make_unique<Dir>();
                                parse_dir(value, *current_dir.subdirs[key]);
                            }
                        } else {
                            // Scalar file
                            string content = value.as<string>("");
                            string perms = "rw-r--r--";
                            current_dir.files[key] = make_unique<File>();
                            current_dir.files[key]->content = content;
                            current_dir.files[key]->perms = perms;
                        }
                    }
                };
                root = make_unique<Dir>();
                parse_dir(yaml_fs, *root);
                cout << "Loaded Linux FS state: scenario '" << scenario << "' (root loaded)\n";
            }
            if (scenario_fs["current_dir"]) current_dir = scenario_fs["current_dir"].as<string>("/home/apprentice");
        } catch (const std::exception &ex) {
            cout << "LinuxModule load error: " << ex.what() << "\n";
            return false;
        }
        registered = {"ls", "cd", "pwd", "cat", "chmod", "touch"};
        cout << "Linux shell ready (commands: " << registered.size() << ")\n";
        return true;
    }

    bool supports_command(const std::string& cmdPrefix) const override {
        return find(registered.begin(), registered.end(), cmdPrefix) != registered.end();
    }

   std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override {
        // Command handlers: prefix -> lambda (args, captures this/out)
        static const unordered_map<string, function<string(const vector<string>&)>> handlers = {
            {"ls", [this](const auto& args) -> string {
                string target = args.empty() ? current_dir : args[0];
                Dir* dir = get_dir(target);
                if (!dir) return "ls: No such directory: " + target + "\n";
                stringstream out;
                for (const auto& f : dir->files) out << f.first << " (" << f.second->perms << ") ";
                for (const auto& d : dir->subdirs) out << d.first << "/ ";
                return out.str();
            }},
            {"cd", [this](const auto& args) -> string {
                if (args.empty()) return "cd: No directory provided\n";
                string target = args[0];
                Dir* dir = get_dir(target);
                if (!dir) return "cd: No such directory: " + target + "\n";
                current_dir = target;  // Absolute (from get_dir)
                return "";
            }},
            {"pwd", [this](const auto& /*args*/) -> string {
                return current_dir + "\n";
            }},
            {"cat", [this](const auto& args) -> string {
                if (args.empty()) return "cat: No file provided\n";
                string file_path = current_dir + "/" + args[0];
                Dir* dir = get_dir(current_dir);
                if (!dir || dir->files.count(args[0]) == 0) return "cat: " + args[0] + ": No such file\n";
                return dir->files[args[0]]->content + "\n";
            }},
            {"chmod", [this](const auto& args) -> string {
                if (args.size() < 2) return "chmod: Usage chmod <perms> <file>\n";
                string perms = args[0], fname = args[1];
                Dir* dir = get_dir(current_dir);
                if (!dir || dir->files.count(fname) == 0) return "chmod: " + fname + ": No such file\n";
                dir->files[fname]->perms = perms;
                return "Permissions updated for " + fname + "\n";
            }},
            {"touch", [this](const auto& args) -> string {
                if (args.empty()) return "touch: No file provided\n";
                string fname = args[0];
                Dir* dir = get_dir(current_dir);
                if (!dir) dir = root.get();  // Fallback to root
                dir->files[fname] = make_unique<File>();
                dir->files[fname]->content = "";
                dir->files[fname]->perms = "rw-r--r--";
                return "";
            }}
        };

        auto handler_it = handlers.find(cmdPrefix);
        if (handler_it != handlers.end()) {
            return handler_it->second(args);
        }
        return "Command supported but not implemented in module\n";
    }

    bool evaluate_condition(const YAML::Node& conditionSpec) override {
        if (!conditionSpec || !conditionSpec["type"]) return false;
        string t = conditionSpec["type"].as<string>();
        if (t == "current_dir") {
            string expect = conditionSpec["dir"].as<string>();
            return current_dir == expect;
        }
        if (t == "file_perm") {
            string file = conditionSpec["file"].as<string>();
            string expect = conditionSpec["perms"].as<string>();
            auto dir_it = filesystem.find(current_dir);
            if (dir_it == filesystem.end() || dir_it->second.find(file) == dir_it->second.end()) return false;
            return dir_it->second[file].second == expect;
        }
        return false;
    }

    vector<string> registered_prefixes() const override { return registered; }

private:
    string path;
    FS filesystem;
    unique_ptr<Dir> root = make_unique<Dir>();
    string current_dir = "/home/apprentice";
    vector<string> registered;
    YAML::Node quests;  // Stub for future


    //  Helper: Get dir node (iterative traversal, handles relative/absolute, trailing /, . / ..)
    Dir* get_dir(const string& path_arg) const {
        string path = path_arg;
        if (!path.empty() && path.back() == '/') path.pop_back();  // Strip trailing /

        if (path.empty() || path == ".") {
            // . or empty = current (absolute)
            path = current_dir;
        }
        if (path == "/") return root.get();
        bool absolute = (!path.empty() && path[0] == '/');
        string full_path = absolute ? path : current_dir + "/" + path;  // Relative/absolute

        // Iterative traversal
        Dir* current = root.get();
        stringstream ss(path);
        string token;
        while (getline(ss, token, '/')) {
            if (token.empty()) continue;
            if (token == ".") continue;  // Skip
            if (token == "..") {
                // Walk up (stub: return current)
                return current;
            }
            if (current->subdirs.count(token) == 0 || !current->subdirs[token]) return nullptr;
            current = current->subdirs[token].get();
        }
        return current;
    }
};

// Factory
std::shared_ptr<SmiteModule> create_module_linux() {
    return std::make_shared<LinuxModule>();
}