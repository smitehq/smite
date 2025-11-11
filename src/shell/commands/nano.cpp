#include "nano.h"
#include "../shell.h"
#include "../nano.h"
#include <memory>

NanoCommand::NanoCommand(Shell* shell) : ShellCommand(shell) {}

std::string NanoCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "nano: missing file operand\n";
    }

    if (args[0] == "-V" || args[0] == "--version") {
        return "GNU nano version 2.0 (simulated)\n";
    }

    if (args[0] == "--help") {
        return "Usage: nano <FILE>\n";
    }

    std::string file_path = shell_->resolve_path(args[0]);
    auto [dir, filename] = shell_->get_dir_and_file(file_path);

    if (!dir) {
        return "nano: cannot access '" + args[0] + "': No such file or directory\n";
    }

    // Get existing content or empty string for new file
    std::string content;
    if (dir->files.count(filename) > 0) {
        content = dir->files[filename]->content;
    }

    // Open nano editor with save callback
    Nano editor;
    bool success = editor.open(filename, content, [&](const std::string& new_content) {
        // Save callback - update the file in the virtual filesystem
        if (dir->files.count(filename) == 0) {
            dir->files[filename] = std::make_unique<File>(File{new_content});
        } else {
            dir->files[filename]->content = new_content;
        }
    });

    if (!success) {
        return "nano: editor error\n";
    }

    return "";  // nano produces no output on successful exit
}

std::string NanoCommand::name() const {
    return "nano";
}

std::string NanoCommand::help() const {
    return "nano <file> - Edit file with nano text editor";
}

std::unique_ptr<ShellCommand> create_nano_command(Shell* shell) {
    return std::make_unique<NanoCommand>(shell);
}