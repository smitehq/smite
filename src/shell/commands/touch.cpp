#include "touch.h"
#include "../shell.h"
#include <memory>

TouchCommand::TouchCommand(Shell* shell) : ShellCommand(shell) {}

std::string TouchCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "touch: missing file operand\n";
    }

    std::string file_path = shell_->resolve_path(args[0]);
    auto [dir, filename] = shell_->get_dir_and_file(file_path);

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

std::string TouchCommand::name() const {
    return "touch";
}

std::string TouchCommand::help() const {
    return "touch <file> - Create an empty file";
}

std::unique_ptr<ShellCommand> create_touch_command(Shell* shell) {
    return std::make_unique<TouchCommand>(shell);
}
