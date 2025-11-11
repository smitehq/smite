// ============================================
// src/shell/commands/cat.h
// ============================================
#ifndef SHELL_COMMANDS_CAT_H
#define SHELL_COMMANDS_CAT_H

#include "shell/shell.h"

namespace shell_commands {

inline auto cmd_cat(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) return "cat: No file provided\n";

        std::string file_path = shell->resolve_path(args[0]);
        auto [dir, filename] = shell->get_dir_and_file(file_path);

        if (!dir || dir->files.count(filename) == 0) {
            return "cat: " + args[0] + ": No such file or directory\n";
        }

        return dir->files[filename]->content + "\n";
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_CAT_H
