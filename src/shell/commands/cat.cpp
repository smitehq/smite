#include "cat.h"
#include "../shell.h"
#include <sstream>

CatCommand::CatCommand(Shell* shell) : ShellCommand(shell) {}

std::string CatCommand::execute(const std::vector<std::string>& args) {
    if (args.empty()) {
        return "cat: No file provided\n";
    }

    std::string file_path = shell_->resolve_path(args[0]);
    auto [dir, filename] = shell_->get_dir_and_file(file_path);

    if (!dir || dir->files.count(filename) == 0) {
        return "cat: " + args[0] + ": No such file or directory\n";
    }

    return dir->files[filename]->content + "\n";
}

std::string CatCommand::name() const {
    return "cat";
}

std::string CatCommand::help() const {
    return "cat <file> - Display file contents";
}

// Factory function
std::unique_ptr<ShellCommand> create_cat_command(Shell* shell) {
    return std::make_unique<CatCommand>(shell);
}


// register_command("cat", [this](const auto& args) -> string {
//         if (args.empty()) return "cat: No file provided\n";
//         string file_path = resolve_path(args[0]);

//         auto [dir, filename] = get_dir_and_file(file_path);
//         if (!dir || dir->files.count(filename) == 0)
//             return "cat: " + file_path + ": No such file\n";

//         return dir->files[filename]->content + "\n";
//     });