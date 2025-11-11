#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>
#include <cctype>

class Utils {
public:
    static std::vector<std::string> tokenize_command(const std::string& cmd);
    static std::string trim(const std::string& str);
};

inline std::vector<std::string> Utils::tokenize_command(const std::string& cmd) {
    std::vector<std::string> tokens;
    std::string current;
    bool in_single = false;
    bool in_double = false;

    for (size_t i = 0; i < cmd.size(); ++i) {
        char c = cmd[i];
        if (c == '\'' && !in_double) {
            in_single = !in_single;
            continue;
        }
        if (c == '"' && !in_single) {
            in_double = !in_double;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c)) && !in_single && !in_double) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }

    if (!current.empty()) tokens.push_back(current);
    return tokens;
}

inline std::string Utils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}
