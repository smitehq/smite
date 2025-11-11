//   register_command("ps", [this](const auto& args) -> std::string {
//         bool aux = !args.empty() && args[0] == "aux";
//         if (!aux) return "Usage: ps aux";

//         std::ostringstream out;
//         out << std::left
//             << std::setw(10) << "USER"
//             << std::setw(6)  << "PID"
//             << std::setw(6)  << "%CPU"
//             << std::setw(6)  << "%MEM"
//             << std::setw(10) << "VSZ"
//             << std::setw(10) << "RSS"
//             << std::setw(6)  << "TTY"
//             << std::setw(6)  << "STAT"
//             << std::setw(10) << "START"
//             << std::setw(6)  << "TIME"
//             << "COMMAND\n";

//         struct Proc { std::string user, pid, cpu, mem, vsz, rss, tty, stat, start, time, cmd; };
//         std::vector<Proc> processes = {
//             {"root", "1", "0.0", "0.1", "22528", "1024", "?", "Ss", "Oct07", "0:03", "init"},
//             {"root", "12", "0.0", "0.0", "0", "0", "?", "S", "Oct07", "0:00", "kthreadd"},
//             {"root", "100", "0.1", "0.2", "204800", "4096", "?", "Ss", "Oct07", "0:12", "sshd: zaphod [priv]"},
//             {"zaphod", "101", "0.0", "0.3", "102400", "5120", "pts/0", "Ss", "Oct07", "0:01", "bash"},
//             {"root", "200", "0.2", "0.5", "409600", "8192", "?", "Sl", "Oct07", "0:23", "kworker/0:1"},
//         };

//         // Output
//         for (auto& p : processes) {
//             out << std::setw(10) << p.user
//                 << std::setw(6)  << p.pid
//                 << std::setw(6)  << p.cpu
//                 << std::setw(6)  << p.mem
//                 << std::setw(10) << p.vsz
//                 << std::setw(10) << p.rss
//                 << std::setw(6)  << p.tty
//                 << std::setw(6)  << p.stat
//                 << std::setw(10) << p.start
//                 << std::setw(6)  << p.time
//                 << p.cmd
//                 << "\n";
//         }

//         return out.str();
//     });