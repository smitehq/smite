#ifndef MODULES_LINUX_MODULE_H
#define MODULES_LINUX_MODULE_H

#include "globals.h"
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <functional>
#include <unordered_map>
#include <yaml-cpp/yaml.h>  // Full YAML for Node (not just fwd decl)
#include "core/module_interface.h"

//--------------------------------------
// Basic filesystem structures
//--------------------------------------
struct File {
    std::string content;
    std::string perms = "rw-r--r--";
};

struct Dir {
    std::map<std::string, std::unique_ptr<Dir>> subdirs;
    std::map<std::string, std::unique_ptr<File>> files;
};

//--------------------------------------
// LinuxModule
//--------------------------------------
class LinuxModule : public SmiteModule {
public:
    LinuxModule() = default;
    ~LinuxModule() override = default;

    //--------------------------------------
    // SmiteModule interface
    //--------------------------------------
    std::string name() const override;
    bool load_from_path(const std::string& modulePath) override;
    bool supports_command(const std::string& cmdPrefix) const override;
    std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    bool evaluate_condition(const YAML::Node& conditionSpec) override;
    std::vector<std::string> registered_prefixes() const override;

    //--------------------------------------
    // Debug/testing
    //--------------------------------------
    size_t fs_size() const;
    std::string fs_debug() const;

private:
    //--------------------------------------
    // Core filesystem state
    //--------------------------------------
    std::string path;
    std::unique_ptr<Dir> root = std::make_unique<Dir>();
    std::string current_dir = "/";
    std::string home = std::string("/home/") + globals::PLAYER_NAME; // default, updated from state.yaml
    YAML::Node quests;  // future use (quests.yaml)
    std::map<std::string, std::map<std::string, std::pair<std::string, std::string>>> fs_tree; // legacy emulation map

    //--------------------------------------
    // Command system
    //--------------------------------------
    using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
    std::unordered_map<std::string, CommandHandler> command_registry;

    void register_builtin_commands();
    void register_command(const std::string& name, CommandHandler handler);
    std::unordered_map<std::string, std::string> alias_registry;
    std::vector<std::string> tokenize_command_string(const std::string& cmd) const;

    //--------------------------------------
    // Helpers for filesystem traversal
    //--------------------------------------
    Dir* get_dir(const std::string& path) const;
    std::pair<Dir*, std::string> get_dir_and_file(const std::string& full_path) const;
    std::string resolve_path(const std::string& path_arg) const;
    std::string expand_home(const std::string& path_arg) const;
};

#endif  // MODULES_LINUX_MODULE_H
