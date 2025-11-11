#ifndef CORE_COMMAND_ROUTER_H
#define CORE_COMMAND_ROUTER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include "module_interface.h"

class Router {
public:
    void add_module(std::shared_ptr<SmiteModule> module);

    // Command dispatch
    std::string handle_command(const std::vector<std::string>& tokens);
    std::vector<std::string> list_commands() const;

    // Module support - return by const reference to avoid copy
    const std::vector<std::shared_ptr<SmiteModule>>& get_modules() const;
    std::shared_ptr<SmiteModule> get_module_by_name(const std::string& name) const;

    // Tab completion support
    std::vector<std::string> complete_command(
        const std::vector<std::string>& tokens, 
        size_t token_index, 
        const std::string& current_token) const;

private:
    std::vector<std::shared_ptr<SmiteModule>> modules_;

    // Precomputed: command prefix -> module
    std::unordered_map<std::string, std::shared_ptr<SmiteModule>> prefix_map_;
    size_t max_prefix_tokens_ = 0;
};

#endif // CORE_COMMAND_ROUTER_H