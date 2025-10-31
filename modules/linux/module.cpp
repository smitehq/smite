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
                function<void(const Node&, string)> parse_dir = [&](const Node& node, string current_path) {
                    for (const auto& entry : node) {
                        string key = entry.first.as<string>();
                        Node value = entry.second;
                        string full_path = current_path.empty() ? "/" + key : current_path + "/" + key;
                        if (value.IsMap()) {
                            // Dir: Init inner, recurse
                            filesystem[full_path];  // Empty inner map
                            parse_dir(value, full_path);
                        } else {
                            // File: Scalar or map
                            string content = value.IsScalar() ? value.as<string>() : value["content"].as<string>("");
                            string perms = value.IsMap() ? value["perms"].as<string>("rw-r--r--") : "rw-r--r--";
                            filesystem[current_path][key] = {content, perms};
                        }
                    }
                };
                parse_dir(yaml_fs, "");
                cout << "Loaded Linux FS state: scenario '" << scenario << "' (dirs: " << filesystem.size() << ")\n";
            }
            if (scenario_fs["current_dir"]) {
                string cd = scenario_fs["current_dir"].as<string>();
                current_dir = cd.empty() ? "/" : "/" + cd;  // Prepend "/"
            }
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
        // Helper: Find file in current dir (always returns inner iterator)
        auto find_file = [this](const string& fname) {
            return filesystem[current_dir].find(fname);  // [] inserts empty inner if missing
        };

        // Command handlers: prefix -> lambda (args, captures this/out)
        static const unordered_map<string, function<string(const vector<string>&)>> handlers = {
            {"ls", [this](const auto& args) -> string {
                stringstream out;
                if (args.empty()) {
                    // Default to current_dir
                    auto dir_it = filesystem.find(current_dir);
                    if (dir_it == filesystem.end()) return "ls: No such directory: " + current_dir + "\n";
                    for (const auto& f : dir_it->second) out << f.first << " (" << f.second.second << ") ";
                } else {
                    // List each arg dir
                    for (const string& arg : args) {
                        auto dir_it = filesystem.find(arg);
                        if (dir_it == filesystem.end()) {
                            out << "ls: No such directory: " << arg << "\n";
                        } else {
                            out << arg + ":\n";
                            for (const auto& f : dir_it->second) out << "  " << f.first << " (" << f.second.second << ")\n";
                        }
                    }
                }
                return out.str();
            }},
            {"cd", [this](const auto& args) -> string {
                if (args.empty()) return "cd: No directory provided\n";
                string target = args[0];
                if (target == "/") current_dir = "/";
                else if (target == "..") {
                    size_t pos = current_dir.find_last_of('/');
                    if (pos != string::npos && pos > 0) current_dir = current_dir.substr(0, pos);
                } else current_dir += (current_dir == "/" ? "" : "/") + target;
                return "";  // Silent
            }},
            {"pwd", [this](const auto& /*args*/) -> string {
                return current_dir + "\n";
            }},
            {"cat", [this, find_file](const auto& args) -> string {
                if (args.empty()) return "cat: No file provided\n";
                string fname = args[0];
                auto file_it = find_file(fname);
                if (file_it == filesystem[current_dir].end()) return "cat: " + fname + ": No such file\n";
                return file_it->second.first + "\n";
            }},
            {"chmod", [this, find_file](const auto& args) -> string {
                if (args.size() < 2) return "chmod: Usage chmod <perms> <file>\n";
                string perms = args[0], fname = args[1];
                auto file_it = find_file(fname);
                if (file_it == filesystem[current_dir].end()) return "chmod: " + fname + ": No such file\n";
                file_it->second.second = perms;
                return "Permissions updated for " + fname + "\n";
            }},
            {"touch", [this](const auto& args) -> string {
                if (args.empty()) return "touch: No file provided\n";
                string fname = args[0];
                filesystem[current_dir][fname] = {"", "rw-r--r--"};
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
    string current_dir = "/home/apprentice";
    vector<string> registered;
    YAML::Node quests;  // Stub for future
};

// Factory
std::shared_ptr<SmiteModule> create_module_linux() {
    return std::make_shared<LinuxModule>();
}