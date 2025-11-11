// ============================================
// src/shell/commands/chmod.h
// ============================================
#ifndef SHELL_COMMANDS_CHMOD_H
#define SHELL_COMMANDS_CHMOD_H

#include "shell/shell.h"

namespace shell_commands {

inline auto cmd_chmod(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
        if (args.size() < 2) {
            return "chmod: missing operand\nUsage: chmod <mode> <file>\n";
        }

        std::string mode = args[0];
        std::string file_path = shell->resolve_path(args[1]);
        auto [dir, filename] = shell->get_dir_and_file(file_path);

        if (!dir || dir->files.count(filename) == 0) {
            return "chmod: cannot access '" + args[1] + "': No such file or directory\n";
        }

        if (mode.size() != 3 && mode.size() != 4) {
            return "chmod: invalid mode: '" + mode + "'\n";
        }

        for (char c : mode) {
            if (c < '0' || c > '7') {
                return "chmod: invalid mode: '" + mode + "'\n";
            }
        }

        dir->files[filename]->perms = mode;
        return "";
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_CHMOD_H