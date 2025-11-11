#include "cd.h"
#include "../shell.h"

CdCommand::CdCommand(Shell* shell) : ShellCommand(shell) {}

std::string CdCommand::execute(const std::vector<std::string>& args) {
    std::string target_path = args.empty() ? shell_->get_home() : shell_->resolve_path(args[0]);
    
    Dir* target_dir = shell_->get_dir(target_path);
    if (!target_dir) {
        return "cd: " + (args.empty() ? "~" : args[0]) + ": No such file or directory\n";
    }

    // Update current directory (need to add setter to Shell)
    shell_->set_current_dir(target_path);
    return "";  // cd produces no output on success
}

std::string CdCommand::name() const {
    return "cd";
}

std::string CdCommand::help() const {
    return "cd [path] - Change working directory";
}

std::unique_ptr<ShellCommand> create_cd_command(Shell* shell) {
    return std::make_unique<CdCommand>(shell);
}


// register_command("cd", [this](const auto& args) -> string {
//         if (args.empty()) return "cd: No directory provided\n";
//         string target = args[0];

//         if (target == "..") {
//             // Go up one directory
//             if (current_dir == "/") return ""; // already root
//             size_t pos = current_dir.find_last_of('/');
//             if (pos == 0) current_dir = "/";
//             else if (pos != string::npos) current_dir = current_dir.substr(0, pos);
//             return "";
//         }

//         string resolved = resolve_path(target);
//         Dir* dir = get_dir(resolved);
//         if (!dir) return "cd: No such directory: " + target + "\n";
//         current_dir = resolved;

//         return "";
//     });
