// ============================================
// src/shell/commands/cd.h
// ============================================
#ifndef SHELL_COMMANDS_CD_H
#define SHELL_COMMANDS_CD_H

#include "shell/shell.h"

namespace shell_commands {

inline auto cmd_cd(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
    if (args.empty()) {
        shell->set_current_dir(shell->get_home());
        return "";
    }

    std::string target = args[0];
    std::optional<std::string> resolved = shell->normalize_path(target);
    if (!resolved) {
        return "cd: No such directory: " + target + "\n";
    }

    shell->set_current_dir(*resolved);
    return "";
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_CD_H
