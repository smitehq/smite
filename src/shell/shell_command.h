
#ifndef SHELL_COMMAND_H
#define SHELL_COMMAND_H

#include <string>
#include <vector>
#include <memory>

// Forward declaration
class Shell;

/**
 * Base class for shell commands.
 * Each command is implemented as a separate class for better organization.
 */
class ShellCommand {
public:
    explicit ShellCommand(Shell* shell) : shell_(shell) {}
    virtual ~ShellCommand() = default;

    /**
     * Execute the command with given arguments.
     * @param args Command arguments (excludes the command name itself)
     * @return Command output as string
     */
    virtual std::string execute(const std::vector<std::string>& args) = 0;

    /**
     * @return The command name (e.g., "ls", "cat", "echo")
     */
    virtual std::string name() const = 0;

    /**
     * @return Short help text describing the command
     */
    virtual std::string help() const = 0;

protected:
    Shell* shell_;  // Non-owning pointer to parent shell for accessing filesystem
};

/**
 * Factory function type for creating commands.
 * Each command implementation provides a create_<name>_command() function.
 */
using CommandFactory = std::unique_ptr<ShellCommand>(*)(Shell*);

#endif // SHELL_COMMAND_H