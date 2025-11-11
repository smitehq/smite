// ============================================
// src/shell/commands/pwd.h
// ============================================
#ifndef SHELL_COMMANDS_PWD_H
#define SHELL_COMMANDS_PWD_H

#include "shell/shell.h"

namespace shell_commands {

inline auto cmd_pwd(Shell* shell) {
    return [shell](const std::vector<std::string>&) -> std::string {
        return shell->get_current_dir() + "\n";
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_PWD_H