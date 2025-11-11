#ifndef SHELL_COMMANDS_LS_H
#define SHELL_COMMANDS_LS_H

#include "../shell_command.h"

class LsCommand : public ShellCommand {
public:
    explicit LsCommand(Shell* shell);

    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

// Factory function
std::unique_ptr<ShellCommand> create_ls_command(Shell* shell);

#endif // SHELL_COMMANDS_LS_H