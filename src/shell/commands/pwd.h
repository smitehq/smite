// ============================================
// pwd.h
// ============================================
#ifndef SHELL_COMMANDS_PWD_H
#define SHELL_COMMANDS_PWD_H

#include "../shell_command.h"

class PwdCommand : public ShellCommand {
public:
    explicit PwdCommand(Shell* shell);
    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

std::unique_ptr<ShellCommand> create_pwd_command(Shell* shell);

#endif // SHELL_COMMANDS_PWD_H