// ============================================
// src/shell/commands/pwd.h
// ============================================
#ifndef SHELL_COMMANDS_PWD_H
#define SHELL_COMMANDS_PWD_H

#include "../shell.h"

namespace shell_commands {

inline std::string pwd(Shell* shell, const std::vector<std::string>&) {
    return shell->get_current_dir() + "\n";
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_PWD_H