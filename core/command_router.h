#ifndef CORE_COMMAND_ROUTER_H
#define CORE_COMMAND_ROUTER_H

#include <string>
#include <vector>
#include <memory>
#include "module_interface.h"  // SmiteModule

class CommandRouter {
public:
    void add_module(std::shared_ptr<SmiteModule> module);
    std::string handle_input(const std::string& raw);
    std::vector<std::string> list_commands() const;
    std::vector<std::shared_ptr<SmiteModule>> get_modules() const;
    static std::vector<std::string> tokenize(const std::string& s);

private:
    std::vector<std::shared_ptr<SmiteModule>> modules;
};

#endif