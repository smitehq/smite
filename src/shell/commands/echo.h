// ============================================
// src/shell/commands/echo.h
// ============================================
#ifndef SHELL_COMMANDS_ECHO_H
#define SHELL_COMMANDS_ECHO_H

#include "shell/shell.h"
#include <sstream>

namespace shell_commands {

inline auto cmd_echo(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
        (void)shell;  // Unused but captured for consistency
        if (args.empty()) return "\n";

        std::ostringstream oss;
        for (size_t i = 0; i < args.size(); ++i) {
            oss << args[i];
            if (i < args.size() - 1) oss << " ";
        }
        oss << "\n";

        return oss.str();
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_ECHO_H