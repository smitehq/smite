// ============================================
// touch.h
// ============================================
#ifndef SHELL_COMMANDS_TOUCH_H
#define SHELL_COMMANDS_TOUCH_H

#include "../shell_command.h"

class TouchCommand : public ShellCommand {
public:
    explicit TouchCommand(Shell* shell);
    std::string execute(const std::vector<std::string>& args) override;
    std::string name() const override;
    std::string help() const override;
};

std::unique_ptr<ShellCommand> create_touch_command(Shell* shell);

#endif // SHELL_COMMANDS_TOUCH_H