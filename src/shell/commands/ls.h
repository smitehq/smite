
// ============================================
// src/shell/commands/ls.h
// ============================================
#ifndef SHELL_COMMANDS_LS_H
#define SHELL_COMMANDS_LS_H

#include "shell/shell.h"
#include <sstream>
#include <algorithm>

namespace shell_commands {

inline auto cmd_ls(Shell* shell) {
    return [shell](const std::vector<std::string>& args) -> std::string {
        std::string target;
        bool long_format = false;
        std::string current_dir = shell->get_current_dir();

        // parse args
        for (const auto& arg : args) {
            if (arg == "-l") long_format = true;
            else if (target.empty()) target = arg; // first non-flag arg is the path
        }
        if (target.empty()) target = current_dir; // default to current dir

        Dir* dir = shell->get_dir(shell->resolve_path(target));
        if (!dir) return "ls: No such directory: " + target + "\n";

        std::stringstream out;

        if (long_format) {
            // simple total (approximate)
            size_t total_blocks = 0;
            for (const auto& f : dir->files) total_blocks += (f.second->content.size() + 511) / 512;
            out << "total " << total_blocks << "\n";

            // directories
            for (const auto& d : dir->subdirs) {
                out << "drwxr-xr-x " << (2 + dir->subdirs.size()) << " " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " 4096 Nov  1 00:00 " << d.first << "\n";
            }

            // files
            for (const auto& f : dir->files) {
                out << f.second->perms << " 1 " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " "
                    << f.second->content.size() << " " << f.second->modified_at << " "
                    << f.first << "\n";
            }
        } else {
            for (const auto& f : dir->files) out << f.first << " ";
            for (const auto& d : dir->subdirs) out << d.first << "/ ";
            out << "\n";
        }

        return out.str();
    };
}

} // namespace shell_commands

#endif // SHELL_COMMANDS_LS_H
