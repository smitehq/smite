#pragma once
#include "router.h"
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include "core/module_interface.h"
#include <chrono>
#include <ctime>
#include <format>

struct File {
    std::string content;
    std::string perms;
    std::string created_at;
    std::string modified_at;

    // Constructor
    File(const std::string& content_ = "", const std::string& perms_ = "rw-r--r--")
        : content(content_), perms(perms_)
    {
        created_at = current_time();
        modified_at = created_at;
    }

    // Set content and automatically update modified_at
    void set_content(const std::string& new_content) {
        content = new_content;
        modified_at = current_time();
    }

private:
    static std::string current_time() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm{};
    #ifdef _WIN32
        localtime_s(&local_tm, &t);
    #else
        localtime_r(&t, &local_tm);
    #endif

        char buf[20];
        strftime(buf, sizeof(buf), "%b %d %H:%M", &local_tm);
        return std::string(buf); // or fmt::format("{}", buf) when C++20 is available
    }
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
    std::string get_current_dir() const { return current_dir; }
    std::string get_home() const { return home; }

    // tab autocompletion for virtual fs
    void setup_readline_completion(Router* router);

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
