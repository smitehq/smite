#ifndef MODULES_KUBERNETES_MODULE_H
#define MODULES_KUBERNETES_MODULE_H

#include "core/module_interface.h"
#include <yaml-cpp/yaml.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>
#include <sstream>
#include <iostream>
#include <algorithm>

struct Pod {
    std::string name;
    std::string ns = "default";
    std::string status;
    int restarts = 0;
    std::vector<std::string> logs;
};

class KubernetesModule : public SmiteModule {
public:
    KubernetesModule() = default;
    ~KubernetesModule() override = default;

    //--------------------------------------
    // SmiteModule interface
    //--------------------------------------
    std::string name() const override;
    bool load_from_path(const std::string& modulePath) override;
    bool supports_command(const std::string& cmdPrefix) const override;
    std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    bool evaluate_condition(const YAML::Node& conditionSpec) override;
    std::vector<std::string> registered_prefixes() const override;

private:
    std::string path;
    std::vector<Pod> pods;
    YAML::Node quests; // raw quests

    //--------------------------------------
    // Command registry
    //--------------------------------------
    using CommandHandler = std::function<std::string(const std::vector<std::string>&)>;
    std::unordered_map<std::string, CommandHandler> command_registry;

    void register_builtin_commands();
    void register_command(const std::string& name, CommandHandler handler);

    //--------------------------------------
    // Helper
    //--------------------------------------
    auto find_pod(const std::string& pod_name) -> decltype(pods.begin());
};

#endif // MODULES_KUBERNETES_MODULE_H
