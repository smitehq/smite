// ============================================
// echo.h
// ============================================
#ifndef SHELL_COMMANDS_ECHO_H
#define SHELL_COMMANDS_ECHO_H

#include "../shell_command.h"

class EchoCommand : public ShellCommand {
public:
    explicit EchoCommand(Shell* shell);
    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

std::unique_ptr<ShellCommand> create_echo_command(Shell* shell);

#endif // SHELL_COMMANDS_ECHO_H