// register_command("df", [this](const auto& args) -> std::string {
//         bool human = !args.empty() && args[0] == "-h";
//         std::ostringstream out;

//         out << std::left
//             << std::setw(12) << "Filesystem"
//             << std::setw(10) << "Size"
//             << std::setw(10) << "Used"
//             << std::setw(10) << "Avail"
//             << std::setw(6)  << "Use%"
//             << "Mounted on\n";

//         // Fake stats
//         struct Fs { std::string name, mount; int size, used; } filesystems[] = {
//             {"/dev/sda1","/",50, 20},
//             {"/dev/sda2","/etc",1, 0},
//             {"/dev/sda3","/var",10, 3},
//             {"/dev/sda4","/home",100, 42},
//         };

//         for (auto& fs : filesystems) {
//             int avail = fs.size - fs.used;
//             int usep = fs.size == 0 ? 0 : (fs.used * 100 / fs.size);
//             out << std::setw(12) << fs.name
//                 << std::setw(10) << (human ? std::to_string(fs.size) + "G" : std::to_string(fs.size))
//                 << std::setw(10) << (human ? std::to_string(fs.used) + "G" : std::to_string(fs.used))
//                 << std::setw(10) << (human ? std::to_string(avail) + "G" : std::to_string(avail))
//                 << std::setw(6)  << (std::to_string(usep) + "%")
//                 << fs.mount << "\n";
//         }

//         return out.str();
//     });