#include "pwd.h"
#include "../shell.h"

PwdCommand::PwdCommand(Shell* shell) : ShellCommand(shell) {}

std::string PwdCommand::execute(const std::vector<std::string>& args) {
    (void)args;  // Unused
    return shell_->get_current_dir() + "\n";
}

std::string PwdCommand::name() const {
    return "pwd";
}

std::string PwdCommand::help() const {
    return "pwd - Print working directory";
}

std::unique_ptr<ShellCommand> create_pwd_command(Shell* shell) {
    return std::make_unique<PwdCommand>(shell);
}