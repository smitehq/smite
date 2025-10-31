#include "command_router.h"
#include <sstream>
#include <algorithm>
#include <sstream>  // For istringstream

void CommandRouter::add_module(std::shared_ptr<SmiteModule> module) {
    modules.push_back(std::move(module));
}

std::vector<std::string> CommandRouter::tokenize(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> t;
    std::string tok;
    while (iss >> tok) t.push_back(tok);
    return t;
}

// Attempt longest-prefix match across modules
std::string CommandRouter::handle_input(const std::string& raw) {
    auto tokens = tokenize(raw);
    if (tokens.empty()) return "";

    // Build progressive prefixes (longest-first)
    for (int len = (int)tokens.size(); len > 0; --len) {
        std::string prefix = tokens[0];
        for (int i=1;i<len;i++) prefix += " " + tokens[i];

        for (auto &m : modules) {
            if (m->supports_command(prefix)) {
                // args are remaining tokens
                std::vector<std::string> args(tokens.begin()+len, tokens.end());
                return m->run_command(prefix, args);
            }
        }
    }
    return ""; // no module handled it
}

std::vector<std::string> CommandRouter::list_commands() const {
    std::vector<std::string> out;
    for (auto &m : modules) {
        auto pfx = m->registered_prefixes();
        out.insert(out.end(), pfx.begin(), pfx.end());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::shared_ptr<SmiteModule>> CommandRouter::get_modules() const { 
    return modules; 
}