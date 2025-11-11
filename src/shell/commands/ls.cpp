#include "ls.h"
#include "../shell.h"
#include <sstream>
#include <algorithm>

LsCommand::LsCommand(Shell* shell) : ShellCommand(shell) {}

std::string LsCommand::execute(const std::vector<std::string>& args) {
    std::string path = args.empty() ? shell_->get_current_dir() : shell_->resolve_path(args[0]);
    
    Dir* dir = shell_->get_dir(path);
    if (!dir) {
        return "ls: cannot access '" + (args.empty() ? "." : args[0]) + "': No such file or directory\n";
    }

    std::ostringstream oss;
    
    // Collect and sort entries
    std::vector<std::string> entries;
    
    // Add subdirectories
    for (const auto& [name, _] : dir->subdirs) {
        entries.push_back(name + "/");
    }
    
    // Add files
    for (const auto& [name, _] : dir->files) {
        entries.push_back(name);
    }
    
    // Sort alphabetically
    std::sort(entries.begin(), entries.end());
    
    // Output
    if (entries.empty()) {
        return "";  // Empty directory
    }
    
    for (const auto& entry : entries) {
        oss << entry << "  ";
    }
    oss << "\n";
    
    return oss.str();
}

std::string LsCommand::name() const {
    return "ls";
}

std::string LsCommand::help() const {
    return "ls [path] - List directory contents";
}

// Factory function
std::unique_ptr<ShellCommand> create_ls_command(Shell* shell) {
    return std::make_unique<LsCommand>(shell);
}

// register_command("ls", [this](const auto& args) -> std::string {
//         std::string target;
//         bool long_format = false;

//         // parse args
//         for (const auto& arg : args) {
//             if (arg == "-l") long_format = true;
//             else if (target.empty()) target = arg; // first non-flag arg is the path
//         }
//         if (target.empty()) target = current_dir; // default to current dir

//         Dir* dir = get_dir(resolve_path(target));
//         if (!dir) return "ls: No such directory: " + target + "\n";

//         std::stringstream out;

//         if (long_format) {
//             // simple total (approximate)
//             size_t total_blocks = 0;
//             for (const auto& f : dir->files) total_blocks += (f.second->content.size() + 511) / 512;
//             out << "total " << total_blocks << "\n";

//             // directories
//             for (const auto& d : dir->subdirs) {
//                 out << "drwxr-xr-x " << (2 + dir->subdirs.size()) << " " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " 4096 Nov  1 00:00 " << d.first << "\n";
//             }

//             // files
//             for (const auto& f : dir->files) {
//                 out << f.second->perms << " 1 " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " "
//                     << f.second->content.size() << " " << f.second->modified_at << " "
//                     << f.first << "\n";
//             }
//         } else {
//             for (const auto& f : dir->files) out << f.first << " ";
//             for (const auto& d : dir->subdirs) out << d.first << "/ ";
//             out << "\n";
//         }

//         return out.str();
//     });