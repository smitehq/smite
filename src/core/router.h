#ifndef CORE_COMMAND_ROUTER_H
#define CORE_COMMAND_ROUTER_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "module_interface.h"  // SmiteModule

class Router {
public:
    void add_module(std::shared_ptr<SmiteModule> module);
    std::string handle_input(const std::string& raw);
    std::vector<std::string> list_commands() const;
    std::vector<std::shared_ptr<SmiteModule>> get_modules() const;
    static std::vector<std::string> tokenize(const std::string& s);

    // tab completion support
    std::vector<std::string> complete_command(const std::vector<std::string>& tokens, size_t token_index, const std::string& current_token) const;

private:
    std::vector<std::shared_ptr<SmiteModule>> modules;

    // Precomputed: command prefix -> module
    std::unordered_map<std::string, std::shared_ptr<SmiteModule>> prefix_map;
    size_t max_prefix_tokens = 0;  // track longest prefix length
};

#endif
