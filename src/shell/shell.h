#ifndef SHELL_H
#define SHELL_H

#include "../core/router.h"
#include "../core/module_interface.h"
#include "types/file.h"
#include "types/dir.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <optional>

class Shell : public SmiteModule {
public:
    Shell();
    virtual ~Shell() = default;

    // Module interface
    std::string name() const override;
    bool load_from_path(const std::string& rootPath) override;
    std::string run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) override;
    bool supports_command(const std::string& cmdPrefix) const override;
    bool evaluate_condition(const YAML::Node& conditionSpec) override;
    std::vector<std::string> registered_prefixes() const override;

    // Filesystem helpers
    std::string resolve_path(const std::string& path_arg) const;
    std::pair<Dir*, std::string> get_dir_and_file(const std::string& full_path) const;
    std::optional<Dir*> get_dir_safe(const std::string& path_arg) const;
    Dir* get_dir(const std::string& path_arg) const;
    
    std::string get_current_dir() const { return current_dir_; }
    std::string get_home() const { return home_; }
    void set_current_dir(const std::string& dir) { current_dir_ = dir; }

    // Tab autocompletion
    void setup_readline_completion(Router* router);

    struct CompletionContext {
        Router* router;
        Shell* shell;
    };

private:
    std::unique_ptr<Dir> root_;
    std::string current_dir_;
    std::string home_;

    // Command handler type: function that takes Shell* and args, returns output
    using CommandHandler = std::function<std::string(Shell*, const std::vector<std::string>&)>;
    
    // Command registry: command name -> handler function
    std::unordered_map<std::string, CommandHandler> command_registry_;

    // Filesystem helpers
    std::string expand_home(const std::string& path_arg) const;

    // Command registration
    void register_command(const std::string& name, CommandHandler handler);
    void register_all_commands();
    void build_base_state();
};

#endif // SHELL_H