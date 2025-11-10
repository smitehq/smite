#include "../globals.h"
#include "utils.h"
#include "shell.h"
#include "nano.h"
#include <iostream>
#include <filesystem>
#include <sstream>
#include <algorithm>
#include <readline/readline.h>

using namespace std;
namespace fs = std::filesystem;

// Pointers to global state for completion
static Router* g_router_for_completion = nullptr;
static Shell* g_shell_for_completion = nullptr;

Shell::Shell() : root(std::make_unique<Dir>()), command_registry(), alias_registry() {}

std::string Shell::name() const {
    return "shell";
}

bool Shell::load_from_path(const std::string&) {
    // prepare base dirs and current/home
    home = std::string("/home/") + globals::PLAYER_NAME;
    current_dir = home;

    build_base_state();
    register_builtin_commands();

    return true;
}

void Shell::build_base_state() {
    // create base directories
    root->subdirs["home"] = make_unique<Dir>();
    root->subdirs["tmp"] = make_unique<Dir>();
    root->subdirs["etc"] = make_unique<Dir>();
    root->subdirs["bin"] = make_unique<Dir>();
    root->subdirs["var"] = make_unique<Dir>();

    // /home/player
    root->subdirs["home"]->subdirs[globals::PLAYER_NAME] = make_unique<Dir>();
    Dir* home_dir = root->subdirs["home"]->subdirs[globals::PLAYER_NAME].get();

    // prepopulate a few files
    home_dir->files["readme.txt"] = make_unique<File>(File{
        "Welcome to smiteOS!\nType 'ls' to see files.\n"
    });

    // /etc/motd
    root->subdirs["etc"]->files["motd"] = make_unique<File>(File{
        "Welcome adventurer.\nYour mission begins here.\n"
    });

    // /bin/help (for flavor)
    root->subdirs["bin"]->files["help"] = make_unique<File>(File{
        "Usage: help <command>\nCurrently supported: ls, cd, cat, echo, pwd, touch.\n"
    });

    // std::cout << "Base shell filesystem initialized at " << current_dir << "\n";
}


// ---------------- Command Registration ----------------

void Shell::register_command(const std::string& name, std::function<std::string(const std::vector<std::string>&)> handler) {
    command_registry[name] = std::move(handler);
}

void Shell::register_builtin_commands() {
    register_command("ls", [this](const auto& args) -> std::string {
        std::string target;
        bool long_format = false;

        // parse args
        for (const auto& arg : args) {
            if (arg == "-l") long_format = true;
            else if (target.empty()) target = arg; // first non-flag arg is the path
        }
        if (target.empty()) target = current_dir; // default to current dir

        Dir* dir = get_dir(resolve_path(target));
        if (!dir) return "ls: No such directory: " + target + "\n";

        std::stringstream out;

        if (long_format) {
            // simple total (approximate)
            size_t total_blocks = 0;
            for (const auto& f : dir->files) total_blocks += (f.second->content.size() + 511) / 512;
            out << "total " << total_blocks << "\n";

            // directories
            for (const auto& d : dir->subdirs) {
                out << "drwxr-xr-x " << (2 + dir->subdirs.size()) << " " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " 4096 Nov  1 00:00 " << d.first << "\n";
            }

            // files
            for (const auto& f : dir->files) {
                out << f.second->perms << " 1 " << globals::PLAYER_NAME << " " << globals::PLAYER_NAME << " "
                    << f.second->content.size() << " " << f.second->modified_at << " "
                    << f.first << "\n";
            }
        } else {
            for (const auto& f : dir->files) out << f.first << " ";
            for (const auto& d : dir->subdirs) out << d.first << "/ ";
            out << "\n";
        }

        return out.str();
    });

    register_command("cd", [this](const auto& args) -> string {
        if (args.empty()) return "cd: No directory provided\n";
        string target = args[0];

        if (target == "..") {
            // Go up one directory
            if (current_dir == "/") return ""; // already root
            size_t pos = current_dir.find_last_of('/');
            if (pos == 0) current_dir = "/";
            else if (pos != string::npos) current_dir = current_dir.substr(0, pos);
            return "";
        }

        string resolved = resolve_path(target);
        Dir* dir = get_dir(resolved);
        if (!dir) return "cd: No such directory: " + target + "\n";
        current_dir = resolved;

        return "";
    });

    register_command("pwd", [this](const auto&) -> string {
        return current_dir + "\n";
    });

    register_command("cat", [this](const auto& args) -> string {
        if (args.empty()) return "cat: No file provided\n";
        string file_path = resolve_path(args[0]);

        auto [dir, filename] = get_dir_and_file(file_path);
        if (!dir || dir->files.count(filename) == 0)
            return "cat: " + file_path + ": No such file\n";

        return dir->files[filename]->content + "\n";
    });

    register_command("touch", [this](const auto& args) -> string {
        if (args.empty()) return "touch: No file provided\n";
        string file_path = resolve_path(args[0]);
        auto [dir, filename] = get_dir_and_file(file_path);

        if (!dir) return "touch: Invalid path\n";
        dir->files[filename] = make_unique<File>(File{""});
        return "";
    });

    register_command("echo", [this](const auto& args) {
        string out;
        for (const auto& a : args) out += a + " ";
        return out + "\n";
    });

    register_command("nano", [this](const auto& args) -> string {
        if (args.empty()) return "nano: No file provided\n";

        if (args[0] == "-V" || args[0] == "--version") {
            return "GNU nano version 2.0 (simulated)\n";
        }

        if (args[0] == "--help") {
            return "Usage: nano <FILE>\n";
        }

        std::string file_path = resolve_path(args[0]);
        auto [dir, filename] = get_dir_and_file(file_path);

        if (!dir) return "nano: " + file_path + ": No such directory\n";

        // Ensure the file exists in the directory
        auto& file_ptr = dir->files[filename];
        if (!file_ptr) file_ptr = std::make_unique<File>(File{"\n"});

        Nano nano;
        nano.open(filename, file_ptr->content, [&](const std::string& new_content) {
            file_ptr->content = new_content;  // update virtual file
        });

        return "\n";
    });

    register_command("df", [this](const auto& args) -> std::string {
        bool human = !args.empty() && args[0] == "-h";
        std::ostringstream out;

        out << std::left
            << std::setw(12) << "Filesystem"
            << std::setw(10) << "Size"
            << std::setw(10) << "Used"
            << std::setw(10) << "Avail"
            << std::setw(6)  << "Use%"
            << "Mounted on\n";

        // Fake stats
        struct Fs { std::string name, mount; int size, used; } filesystems[] = {
            {"/dev/sda1","/",50, 20},
            {"/dev/sda2","/etc",1, 0},
            {"/dev/sda3","/var",10, 3},
            {"/dev/sda4","/home",100, 42},
        };

        for (auto& fs : filesystems) {
            int avail = fs.size - fs.used;
            int usep = fs.size == 0 ? 0 : (fs.used * 100 / fs.size);
            out << std::setw(12) << fs.name
                << std::setw(10) << (human ? std::to_string(fs.size) + "G" : std::to_string(fs.size))
                << std::setw(10) << (human ? std::to_string(fs.used) + "G" : std::to_string(fs.used))
                << std::setw(10) << (human ? std::to_string(avail) + "G" : std::to_string(avail))
                << std::setw(6)  << (std::to_string(usep) + "%")
                << fs.mount << "\n";
        }

        return out.str();
    });

    register_command("uname", [this](const auto& args) -> std::string {
        bool all = !args.empty() && args[0] == "-a";
        if (all) {
            return "Linux lappy486 6.7.0-smite #1 SMP Thu Nov 7 2025 x86_64 GNU/Linux\n";
        }
        return "Linux\n";
    });

    register_command("ps", [this](const auto& args) -> std::string {
        bool aux = !args.empty() && args[0] == "aux";
        if (!aux) return "Usage: ps aux";

        std::ostringstream out;
        out << std::left
            << std::setw(10) << "USER"
            << std::setw(6)  << "PID"
            << std::setw(6)  << "%CPU"
            << std::setw(6)  << "%MEM"
            << std::setw(10) << "VSZ"
            << std::setw(10) << "RSS"
            << std::setw(6)  << "TTY"
            << std::setw(6)  << "STAT"
            << std::setw(10) << "START"
            << std::setw(6)  << "TIME"
            << "COMMAND\n";

        struct Proc { std::string user, pid, cpu, mem, vsz, rss, tty, stat, start, time, cmd; };
        std::vector<Proc> processes = {
            {"root", "1", "0.0", "0.1", "22528", "1024", "?", "Ss", "Oct07", "0:03", "init"},
            {"root", "12", "0.0", "0.0", "0", "0", "?", "S", "Oct07", "0:00", "kthreadd"},
            {"root", "100", "0.1", "0.2", "204800", "4096", "?", "Ss", "Oct07", "0:12", "sshd: zaphod [priv]"},
            {"zaphod", "101", "0.0", "0.3", "102400", "5120", "pts/0", "Ss", "Oct07", "0:01", "bash"},
            {"root", "200", "0.2", "0.5", "409600", "8192", "?", "Sl", "Oct07", "0:23", "kworker/0:1"},
        };

        // Output
        for (auto& p : processes) {
            out << std::setw(10) << p.user
                << std::setw(6)  << p.pid
                << std::setw(6)  << p.cpu
                << std::setw(6)  << p.mem
                << std::setw(10) << p.vsz
                << std::setw(10) << p.rss
                << std::setw(6)  << p.tty
                << std::setw(6)  << p.stat
                << std::setw(10) << p.start
                << std::setw(6)  << p.time
                << p.cmd
                << "\n";
        }

        return out.str();
    });

}

// ---------------- Path Utilities ----------------

std::string Shell::expand_home(const std::string& path_arg) const {
    if (path_arg.empty()) return path_arg;
    if (path_arg[0] == '~') {
        if (path_arg.size() == 1 || path_arg[1] == '/')
            return home + path_arg.substr(1);
    }
    return path_arg;
}

std::string Shell::resolve_path(const std::string& path_arg) const {
    if (path_arg.empty()) return current_dir;

    std::string path = expand_home(path_arg);

    if (path[0] == '/') return path; // absolute
    if (current_dir == "/") return "/" + path;
    return current_dir + "/" + path;
}

pair<Dir*, string> Shell::get_dir_and_file(const std::string& full_path) const {
    string path = full_path;
    if (path.empty() || path == "/") return {root.get(), ""};

    size_t pos = path.find_last_of('/');
    string dir_part = (pos == string::npos) ? "" : path.substr(0, pos);
    string filename = (pos == string::npos) ? path : path.substr(pos + 1);

    Dir* dir = get_dir(dir_part.empty() ? current_dir : dir_part);
    return {dir, filename};
}

Dir* Shell::get_dir(const string& path_arg) const {
    if (path_arg.empty() || path_arg == "/") return root.get();

    vector<string> parts;
    stringstream ss(path_arg[0] == '/' ? path_arg.substr(1) : path_arg);
    string token;
    while (getline(ss, token, '/')) {
        if (token.empty() || token == ".") continue;
        parts.push_back(token);
    }

    Dir* current = root.get();
    for (const auto& part : parts) {
        if (current->subdirs.count(part) == 0) return nullptr;
        current = current->subdirs.at(part).get();
    }

    return current;
}

// ---------------- Module Interface ----------------

std::string Shell::run_command(const std::string& cmdPrefix, const std::vector<std::string>& args) {
    std::string actualCmd = cmdPrefix;
    std::vector<std::string> actualArgs = args;

    auto it = command_registry.find(actualCmd);
    if (it == command_registry.end())
        return "Command not found: " + actualCmd + "\n";
    return it->second(actualArgs);
}


bool Shell::evaluate_condition(const YAML::Node&) { return false; }

std::vector<std::string> Shell::registered_prefixes() const {
    std::vector<std::string> out;
    for (const auto& kv : command_registry) out.push_back(kv.first);
    return out;
}

bool Shell::supports_command(const std::string& cmdPrefix) const {
    return command_registry.find(cmdPrefix) != command_registry.end();
}

// ---------------- Factory ----------------

std::shared_ptr<SmiteModule> create_module_shell() {
    return std::make_shared<Shell>();
}

// ---------------- Autocompletion ----------------

char* universal_generator(const char* text, int state) {
    static std::vector<std::string> matches;
    static size_t index;

    if (state == 0) {
        matches.clear();
        index = 0;

        // Current line in readline
        std::string line(rl_line_buffer);
        auto tokens = Utils::tokenize_command(line);
        bool after_space = !line.empty() && isspace(line.back());

        // Determine token_index and current_token
        size_t token_index;
        std::string current_token;
        if (tokens.empty() || after_space) {
            token_index = tokens.size(); // new token
            current_token = "";
        } else {
            token_index = tokens.size() - 1;
            current_token = tokens.back();
        }

        // --- Step 1: Command completion ---
        if (g_router_for_completion) {
            auto cmd_matches = g_router_for_completion->complete_command(tokens, token_index, current_token);
            matches.insert(matches.end(), cmd_matches.begin(), cmd_matches.end());
        }

        // --- Step 2: Filesystem completion ---
        // Only attempt if no command completions OR if user is typing arguments (token_index > 0)
        if (matches.empty() && g_shell_for_completion && token_index > 0) {
            auto dir = g_shell_for_completion->get_dir(g_shell_for_completion->get_current_dir());
            if (dir) {
                // Files
                for (const auto& [name, file] : dir->files) {
                    if (name.rfind(current_token, 0) == 0)
                        matches.push_back(name);
                }
                // Subdirectories
                for (const auto& [name, subdir] : dir->subdirs) {
                    if (name.rfind(current_token, 0) == 0)
                        matches.push_back(name + "/");
                }
            }
        }

        // Sort and remove duplicates
        std::sort(matches.begin(), matches.end());
        matches.erase(std::unique(matches.begin(), matches.end()), matches.end());
    }

    // Return the next match
    if (index < matches.size()) {
        return strdup(matches[index++].c_str()); // readline will free
    }

    return nullptr;
}

// Setup readline to only use virtual shell files for completion
// --- Setup readline completion ---
void Shell::setup_readline_completion(Router* router) {
    g_router_for_completion = router;
    g_shell_for_completion = this;

    rl_attempted_completion_function = [](const char* text, int start, int end) -> char** {
        return rl_completion_matches(text, universal_generator);
    };
    rl_completion_entry_function = universal_generator; // use our virtual files
    rl_completion_append_character = ' ';
    rl_basic_word_break_characters = " \t\n";
}