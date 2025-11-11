#include "cd.h"
#include "../shell.h"

CdCommand::CdCommand(Shell* shell) : ShellCommand(shell) {}

std::string CdCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        shell_->set_current_dir(shell_->get_home());
        return "";
    }

    std::string target = args[0];

    // Handle going up one directory
    if (target == "..") {
        std::string current = shell_->get_current_dir();
        if (current == "/") return ""; // already root
        size_t pos = current.find_last_of('/');
        if (pos == 0)
            shell_->set_current_dir("/");
        else if (pos != std::string::npos)
            shell_->set_current_dir(current.substr(0, pos));
        return "";
    }

    // Normal path resolution
    std::string resolved = shell_->resolve_path(target);
    Dir* dir = shell_->get_dir(resolved);
    if (!dir) {
        return "cd: No such directory: " + target + "\n";
    }

    shell_->set_current_dir(resolved);
    return "";
}

std::string CdCommand::name() const {
    return "cd";
}

std::string CdCommand::help() const {
    return "cd [path] - Change working directory";
}

std::unique_ptr<ShellCommand> create_cd_command(Shell* shell) {
    return std::make_unique<CdCommand>(shell);
}
