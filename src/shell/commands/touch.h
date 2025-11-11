// ============================================
// src/shell/commands/touch.h
// ============================================
#ifndef SHELL_COMMANDS_TOUCH_H
#define SHELL_COMMANDS_TOUCH_H

#include "../shell.h"
#include <memory>

namespace shell_commands {

inline std::string touch(Shell* shell, const std::vector<std::string>& args) {
    if (args.empty()) {
        return "touch: missing file operand\n";
    }

    std::string file_path = shell->resolve_path(args[0]);
    auto [dir, filename] = shell->get_dir_and_file(file_path);

    if (!dir) {
        return "touch: cannot touch '" + args[0] + "': No such file or directory\n";
    }

    // Check if file already exists
    if (dir->files.count(filename) > 0) {
        // In real touch, this would update timestamp, but we'll just succeed silently
        return "";
    }

    // Create new empty file
    dir->files[filename] = std::make_unique<File>(File{""});
    return "";
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_TOUCH_H
