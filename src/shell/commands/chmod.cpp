// ============================================
// chmod.cpp - Change File Permissions Command
// ============================================
#include "chmod.h"
#include "../shell.h"

ChmodCommand::ChmodCommand(Shell* shell) : ShellCommand(shell) {}

std::string ChmodCommand::execute(const std::vector<std::string>& args) {
    if (args.size() < 2) {
        return "chmod: missing operand\nUsage: chmod <mode> <file>\n";
    }

    std::string mode = args[0];
    std::string file_path = shell_->resolve_path(args[1]);
    auto [dir, filename] = shell_->get_dir_and_file(file_path);

    if (!dir || dir->files.count(filename) == 0) {
        return "chmod: cannot access '" + args[1] + "': No such file or directory\n";
    }

    // Validate mode (basic validation for octal like 644, 755)
    if (mode.size() != 3 && mode.size() != 4) {
        return "chmod: invalid mode: '" + mode + "'\n";
    }

    for (char c : mode) {
        if (c < '0' || c > '7') {
            return "chmod: invalid mode: '" + mode + "'\n";
        }
    }

    // Set permissions on file
    dir->files[filename]->perms = mode;
    return "";
}

std::string ChmodCommand::name() const {
    return "chmod";
}

std::string ChmodCommand::help() const {
    return "chmod <mode> <file> - Change file permissions";
}

std::unique_ptr<ShellCommand> create_chmod_command(Shell* shell) {
    return std::make_unique<ChmodCommand>(shell);
}