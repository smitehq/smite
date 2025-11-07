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
#include "core/shell.h"

//--------------------------------------
// LinuxModule
//--------------------------------------
class LinuxModule : public Shell {
public:
    LinuxModule() = default;
    ~LinuxModule() override = default;

    //--------------------------------------
    // SmiteModule interface
    //--------------------------------------
    std::string name() const override;
    bool load_from_path(const std::string& modulePath) override;
    // bool supports_command(const std::string& cmdPrefix) const override;
    // std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    // bool evaluate_condition(const YAML::Node& conditionSpec) override;
    // std::vector<std::string> registered_prefixes() const override;

};

#endif  // MODULES_LINUX_MODULE_H
