#include "echo.h"
#include "../shell.h"
#include <sstream>

EchoCommand::EchoCommand(Shell* shell) : ShellCommand(shell) {}

std::string EchoCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "\n";
    }

    std::ostringstream oss;
    for (size_t i = 0; i < args.size(); ++i) {
        oss << args[i];
        if (i < args.size() - 1) {
            oss << " ";
        }
    }
    oss << "\n";
    
    return oss.str();
}

std::string EchoCommand::name() const {
    return "echo";
}

std::string EchoCommand::help() const {
    return "echo [args...] - Display a line of text";
}

// Factory function
std::unique_ptr<ShellCommand> create_echo_command(Shell* shell) {
    return std::make_unique<EchoCommand>(shell);
}