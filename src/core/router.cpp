#include "router.h"
#include "utils.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <fmt/core.h>

void Router::add_module(std::shared_ptr<SmiteModule> module) {
    if (!module) {
        std::cerr << "Error: Attempted to add null module to router\n";
        return;
    }

    modules_.push_back(module);

    // Precompute prefix → module map
    for (const auto& prefix : module->registered_prefixes()) {
        if (prefix.empty()) {
            std::cerr << "Warning: Module " << module->name() 
                      << " registered empty prefix\n";
            continue;
        }

        prefix_map_[prefix] = module;

        // Track longest prefix in tokens
        size_t tok_count = Utils::tokenize_command(prefix).size();
        if (tok_count > max_prefix_tokens_) {
            max_prefix_tokens_ = tok_count;
        }
    }
}

// Optimized longest-prefix matching using precomputed map
std::string Router::handle_command(const std::vector<std::string>& tokens) {
    if (tokens.empty()) return "";

    // Try longest possible prefixes first, down to 1 token
    size_t try_len = std::min(tokens.size(), max_prefix_tokens_);
    
    while (try_len > 0) {
        // Use ostringstream for efficient string building
        std::ostringstream prefix_builder;
        prefix_builder << tokens[0];
        for (size_t i = 1; i < try_len; ++i) {
            prefix_builder << " " << tokens[i];
        }
        std::string prefix = prefix_builder.str();

        auto it = prefix_map_.find(prefix);
        if (it != prefix_map_.end()) {
            std::vector<std::string> args(tokens.begin() + try_len, tokens.end());
            return it->second->run_command(prefix, args);
        }

        --try_len;
    }

    return fmt::format("-bash: {}: command not found\n", tokens[0]);
}

std::vector<std::string> Router::list_commands() const {
    std::vector<std::string> out;
    out.reserve(prefix_map_.size());  // Pre-allocate
    
    for (const auto& [prefix, _] : prefix_map_) {
        out.push_back(prefix);
    }
    
    std::sort(out.begin(), out.end());
    return out;
}

const std::vector<std::shared_ptr<SmiteModule>>& Router::get_modules() const { 
    return modules_; 
}

std::shared_ptr<SmiteModule> Router::get_module_by_name(const std::string& name) const {
    if (name.empty()) return nullptr;
    
    for (const auto& mod : modules_) {
        if (mod && mod->name() == name) {
            return mod;
        }
    }
    return nullptr;
}

std::vector<std::string> Router::complete_command(
    const std::vector<std::string>& tokens, 
    size_t token_index, 
    const std::string& current_token) const {
    
    std::vector<std::string> results;
    results.reserve(prefix_map_.size());  // Reasonable estimate

    for (const auto& [prefix, module] : prefix_map_) {
        auto cmd_tokens = Utils::tokenize_command(prefix);

        // Skip if typed tokens are longer than the command
        if (token_index >= cmd_tokens.size()) continue;

        // Check if all tokens so far match the command prefix
        bool match = true;
        for (size_t i = 0; i < token_index; ++i) {
            if (i >= tokens.size() || cmd_tokens[i] != tokens[i]) {
                match = false;
                break;
            }
        }
        if (!match) continue;

        // Offer completion for the current token
        const std::string& candidate = cmd_tokens[token_index];
        if (candidate.size() >= current_token.size() && 
            candidate.compare(0, current_token.size(), current_token) == 0) {
            results.push_back(candidate);
        }
    }

    // Remove duplicates
    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());
    
    return results;
}