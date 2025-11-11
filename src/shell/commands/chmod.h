// ============================================
// chmod.h
// ============================================
#ifndef SHELL_COMMANDS_CHMOD_H
#define SHELL_COMMANDS_CHMOD_H

#include "../shell_command.h"

class ChmodCommand : public ShellCommand {
public:
    explicit ChmodCommand(Shell* shell);
    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

std::unique_ptr<ShellCommand> create_chmod_command(Shell* shell);

#endif // SHELL_COMMANDS_CHMOD_H