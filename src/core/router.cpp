#include "router.h"
#include <sstream>
#include <algorithm>
#include <iostream>
#include <readline/readline.h>
#include <readline/history.h>
#include <cstdlib> // for strdup

// needed for readline completion
static Router* g_router_for_completion = nullptr;

void Router::add_module(std::shared_ptr<SmiteModule> module) {
    modules.push_back(module);

    // Precompute prefix → module map
    for (const auto& prefix : module->registered_prefixes()) {
        prefix_map[prefix] = module;

        // Track longest prefix in tokens
        size_t tok_count = tokenize(prefix).size();
        if (tok_count > max_prefix_tokens) max_prefix_tokens = tok_count;
    }
}

std::vector<std::string> Router::tokenize(const std::string& s) {
    std::istringstream iss(s);
    std::vector<std::string> t;
    std::string tok;
    while (iss >> tok) t.push_back(tok);
    return t;
}

// Optimized longest-prefix matching using precomputed map
std::string Router::handle_input(const std::string& raw) {
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

std::vector<std::string> Router::list_commands() const {
    std::vector<std::string> out;
    for (auto &m : modules) {
        auto pfx = m->registered_prefixes();
        out.insert(out.end(), pfx.begin(), pfx.end());
    }
    std::sort(out.begin(), out.end());
    return out;
}

std::vector<std::shared_ptr<SmiteModule>> Router::get_modules() const { 
    return modules; 
}

std::vector<std::string> Router::complete_command(const std::vector<std::string>& tokens, size_t token_index, const std::string& current_token) const {
    std::vector<std::string> results;

    for (const auto& [prefix, module] : prefix_map) {
        auto cmd_tokens = tokenize(prefix);

        // skip if typed tokens are longer than the command
        if (token_index >= cmd_tokens.size()) continue;

        // check if all tokens so far match the command prefix
        bool match = true;
        for (size_t i = 0; i < token_index; ++i) {
            if (i >= tokens.size() || cmd_tokens[i] != tokens[i]) {
                match = false;
                break;
            }
        }
        if (!match) continue;

        // offer completion for the current token
        if (cmd_tokens[token_index].rfind(current_token, 0) == 0) {
            results.push_back(cmd_tokens[token_index]);
        }
    }

    std::sort(results.begin(), results.end());
    results.erase(std::unique(results.begin(), results.end()), results.end());
    return results;
}


// Called by readline to generate completions
char* command_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t index;

    if (state == 0) { // first call for this completion
        matches.clear();
        index = 0;

        std::string line(rl_line_buffer);
        auto tokens = Router::tokenize(line);

        // Determine if cursor is after a space
        bool after_space = !line.empty() && isspace(line.back());
        size_t token_index;
        std::string current_token;

        if (tokens.empty() || after_space) {
            token_index = tokens.size(); // new token
            current_token = "";
        } else {
            token_index = tokens.size() - 1;
            current_token = tokens.back();
        }

        // Get completions from the router
        matches = g_router_for_completion->complete_command(tokens, token_index, current_token);
    }

    if (index < matches.size()) {
        return strdup(matches[index++].c_str()); // readline takes ownership
    }

    return nullptr; // no more matches
}


void Router::setup_readline_completion() {
    g_router_for_completion = this;
    rl_attempted_completion_function = [](const char* text, int, int) -> char** {
        return rl_completion_matches(text, command_generator);
    };
    rl_completion_append_character = ' ';
    rl_basic_word_break_characters = " \t\n"; // space and tab delimit words
}
