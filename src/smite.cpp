#include "globals.h"
#include "core/engine.h"
#include "core/router.h"
#include "core/shell.h"
#include <memory>
#include <iostream>

// For prototype simplicity we will statically instantiate the Kubernetes module factory.
// In practice you might dynamically dlopen plugins or map module folder names to factories.

std::shared_ptr<SmiteModule> create_module_shell();
std::shared_ptr<SmiteModule> create_module_linux();
std::shared_ptr<SmiteModule> create_module_kubernetes();

int main(int argc, char** argv) {
    std::setlocale(LC_ALL, "C");  // Sets global C locale safely (no throw)

    // Early argument parsing
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--version" || arg1 == "-v") {
            std::cout << globals::GAME_NAME << " " << globals::GAME_VERSION << "\n";
            return 0;
        }
        // Could add other flags: --help, --list-modules, etc.
    }

    fmt::print(globals::style::header, "{} Welcome, Apprentice. Your journey begins.\n", globals::style::smite);
    std::cout << "smite.sh engine - type 'help' for commands, 'quit' to exit\n" << std::endl;

    // create router & engine and register modules
    Engine engine("./modules"); // modules folder path (ignored for static proto)

    auto shell = create_module_shell();  // Always first
    if (!shell->load_from_path("./src/core/shell")) {
        std::cerr << "Failed to load shell module\n";
        return 1;
    }
    engine.add_module(shell);

    auto linux = create_module_linux();  // Always first
    if (!linux->load_from_path("./src/modules/linux")) {
        std::cerr << "Failed to load linux module\n";
        return 1;
    }
    engine.add_module(linux);

    // instantiate kube module and load its path
    auto kube = create_module_kubernetes();
    if (!kube->load_from_path("./src/modules/kubernetes")) {
        std::cerr << "Failed to load kubernetes module\n";
        return 1;
    }
    engine.add_module(kube);
    // add to router (Engine internally could add modules; here we add it manually)
    // For this simple wiring, we will create our own Router and Engine loop
    // but to keep things short, assume Engine has a method to accept modules (or expand accordingly).
    // For brevity, call engine.repl() after appropriate wiring.
    // You can adapt per earlier core design.

    // (Left as exercise to wire router->engine since many variants possible)
    engine.repl();
    return 0;
}

