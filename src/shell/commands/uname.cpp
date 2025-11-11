// register_command("uname", [this](const auto& args) -> std::string {
//         bool all = !args.empty() && args[0] == "-a";
//         if (all) {
//             return "Linux lappy486 6.7.0-smite #1 SMP Thu Nov 7 2025 x86_64 GNU/Linux\n";
//         }
//         return "Linux\n";
//     });