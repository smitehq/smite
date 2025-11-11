// ============================================
// cd.h
// ============================================
#ifndef SHELL_COMMANDS_CD_H
#define SHELL_COMMANDS_CD_H

#include "../shell_command.h"

class CdCommand : public ShellCommand {
public:
    explicit CdCommand(Shell* shell);
    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

std::unique_ptr<ShellCommand> create_cd_command(Shell* shell);

#endif // SHELL_COMMANDS_CD_H