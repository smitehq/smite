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

// ---------------- LinuxModule ----------------

string LinuxModule::name() const {
    return "linux";
}

bool LinuxModule::load_from_path(const std::string& modulePath) {
/*     path = modulePath;
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

    cout << "Linux shell ready (commands: " << command_registry.size() << ")\n"; */
    return true;
}

std::shared_ptr<SmiteModule> create_module_linux() {
    return std::make_shared<LinuxModule>();
}
