#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include "core/module_interface.h"

struct File {
    std::string content;
    std::string perms;
};

struct Dir {
    std::unordered_map<std::string, std::unique_ptr<Dir>> subdirs;
    std::unordered_map<std::string, std::unique_ptr<File>> files;
};

class Shell : public SmiteModule {
public:
    Shell();
    virtual ~Shell() = default;

    // core interface
    virtual std::string name() const override;
    virtual bool load_from_path(const std::string& rootPath) override;
    virtual std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    virtual bool supports_command(const std::string& cmdPrefix) const override;
    virtual bool evaluate_condition(const YAML::Node& conditionSpec) override;
    virtual std::vector<std::string> registered_prefixes() const override;

    // filesystem helpers
    std::string resolve_path(const std::string& path_arg) const;
    std::pair<Dir*, std::string> get_dir_and_file(const std::string& full_path) const;
    Dir* get_dir(const std::string& path_arg) const;

protected:
    std::unique_ptr<Dir> root;
    std::string current_dir;
    std::string home;

    // shell state
    std::unordered_map<std::string, std::function<std::string(const std::vector<std::string>&)>> command_registry;
    std::unordered_map<std::string, std::string> alias_registry;

    // filesystem helpers
    std::string expand_home(const std::string& path_arg) const;

    // command registration
    void register_command(const std::string& name, std::function<std::string(const std::vector<std::string>&)> handler);

    void register_builtin_commands();
    void build_base_state();
};
