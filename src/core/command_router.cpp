#include "command_router.h"
#include <sstream>
#include <algorithm>
#include <iostream>

void CommandRouter::add_module(std::shared_ptr<SmiteModule> module) {
    modules.push_back(module);

    // Precompute prefix → module map
    for (const auto& prefix : module->registered_prefixes()) {
        prefix_map[prefix] = module;

        // Track longest prefix in tokens
        size_t tok_count = tokenize(prefix).size();
        if (tok_count > max_prefix_tokens) max_prefix_tokens = tok_count;
    }
}

std::vector<std::string> CommandRouter::tokenize(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> t;
    std::string tok;
    while (iss >> tok) t.push_back(tok);
    return t;
}

// Optimized longest-prefix matching using precomputed map
std::string CommandRouter::handle_input(const std::string& raw) {
    auto tokens = tokenize(raw);
    if (tokens.empty()) return "";

    // Try longest possible prefixes first, down to 1 token
    size_t try_len = std::min(tokens.size(), max_prefix_tokens);
    while (try_len > 0) {
        std::string prefix = tokens[0];
        for (size_t i = 1; i < try_len; ++i) prefix += " " + tokens[i];

        auto it = prefix_map.find(prefix);
        if (it != prefix_map.end()) {
            std::vector<std::string> args(tokens.begin() + try_len, tokens.end());
            return it->second->run_command(prefix, args);
        }

        --try_len;
    }

    return ""; // No module handled it
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
