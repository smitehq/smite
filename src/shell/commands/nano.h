// ============================================
// src/shell/commands/nano_cmd.h
// ============================================
#ifndef SHELL_COMMANDS_NANO_CMD_H
#define SHELL_COMMANDS_NANO_CMD_H

#include "shell/shell.h"
#include "shell/nano.h"
#include <memory>

namespace shell_commands {

inline auto cmd_nano(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
        if (args.empty()) {
            return "nano: missing file operand\n";
        }

        if (args[0] == "-V" || args[0] == "--version") {
            return "GNU nano version 2.0 (simulated)\n";
        }

        if (args[0] == "--help") {
            return "Usage: nano <FILE>\n";
        }

        std::string file_path = shell->resolve_path(args[0]);
        auto [dir, filename] = shell->get_dir_and_file(file_path);

        if (!dir) {
            return "nano: cannot access '" + args[0] + "': No such file or directory\n";
        }

        // Get existing content or empty string for new file
        std::string content;
        if (dir->files.count(filename) > 0) {
            content = dir->files[filename]->content;
        }

        // Open nano editor with save callback
        Nano editor;
        bool success = editor.open(filename, content, [&](const std::string& new_content) {
            // Save callback - update the file in the virtual filesystem
            if (dir->files.count(filename) == 0) {
                dir->files[filename] = std::make_unique<File>(File{new_content});
            } else {
                dir->files[filename]->content = new_content;
            }
        });

        if (!success) {
            return "nano: editor error\n";
        }

        return "";  // nano produces no output on successful exit
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_NANO_CMD_H
