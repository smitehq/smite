// ============================================
// src/shell/commands/cd.h
// ============================================
#ifndef SHELL_COMMANDS_CD_H
#define SHELL_COMMANDS_CD_H

#include "../shell.h"

namespace shell_commands {

inline auto cmd_cd(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
    if (args.empty()) {
        shell->set_current_dir(shell->get_home());
        return "";
    }

    std::string target = args[0];

    // Handle going up one directory
    if (target == "..") {
        std::string current = shell->get_current_dir();
        if (current == "/") return ""; // already root
        size_t pos = current.find_last_of('/');
        if (pos == 0)
            shell->set_current_dir("/");
        else if (pos != std::string::npos)
            shell->set_current_dir(current.substr(0, pos));
        return "";
    }

    // Normal path resolution
    std::string resolved = shell->resolve_path(target);
    Dir* dir = shell->get_dir(resolved);
    if (!dir) {
        return "cd: No such directory: " + target + "\n";
    }

    shell->set_current_dir(resolved);
    return "";
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_CD_H
