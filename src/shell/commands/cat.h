#ifndef SHELL_COMMANDS_CAT_H
#define SHELL_COMMANDS_CAT_H

#include "../shell_command.h"

class CatCommand : public ShellCommand {
public:
    explicit CatCommand(Shell* shell);

    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

// Factory function
std::unique_ptr<ShellCommand> create_cat_command(Shell* shell);

#endif // SHELL_COMMANDS_CAT_H