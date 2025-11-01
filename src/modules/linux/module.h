#ifndef MODULES_LINUX_MODULE_H
#define MODULES_LINUX_MODULE_H

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <yaml-cpp/yaml.h>  // Full YAML for Node (not forward decl)
#include "core/module_interface.h"

struct File {
    std::string content;
    std::string perms = "rw-r--r--";
};

struct Dir {
    std::map<std::string, std::unique_ptr<Dir>> subdirs;
    std::map<std::string, std::unique_ptr<File>> files;
};

class LinuxModule : public SmiteModule {
public:
    LinuxModule() = default;
    ~LinuxModule() override = default;

    std::string name() const override;
    bool load_from_path(const std::string& modulePath) override;
    bool supports_command(const std::string& cmdPrefix) const override;
    std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    bool evaluate_condition(const YAML::Node& conditionSpec) override;
    std::vector<std::string> registered_prefixes() const override;

    // Debug getters (for testing)
    size_t fs_size() const;
    std::string fs_debug() const;

private:
    std::string path;
    std::unique_ptr<Dir> root = std::make_unique<Dir>();
    std::string current_dir = "/";
    std::vector<std::string> registered;
    YAML::Node quests;  // Stub for future
    std::map<std::string, std::map<std::string, std::pair<std::string, std::string>>> fs_tree; // dir -> {file: (content, perms)}

    Dir* get_dir(const std::string& path) const;  // Private helper
};

#endif