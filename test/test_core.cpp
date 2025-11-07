#define CATCH_CONFIG_MAIN  // Catch2 self-tests
#include "catch_amalgamated.hpp"
#include "../src/core/engine.h"
#include "../src/core/router.h"
#include "../src/core/module_interface.h"
#include "../src/modules/linux/module.h"
#include <memory>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#include <filesystem>
#include <fstream>

using namespace std;
namespace fs = std::filesystem;
using YAML::Node;

// Stub Module for Tests (implements interface)
class StubModule : public SmiteModule {
public:
    string name() const override { return "stub"; }
    bool load_from_path(const std::string& path) override { return true; }
    bool supports_command(const std::string& prefix) const override { return prefix == "test_cmd"; }
    string run_command(const std::string& prefix, const std::vector<std::string>& args) override {
        if (prefix == "test_cmd") return "test output: " + std::to_string(args.size());
        return "Unknown";
    }
    bool evaluate_condition(const YAML::Node& cond) override { return cond["test"] ? true : false; }
    vector<string> registered_prefixes() const override { return {"test_cmd"}; }
};

// Trim helper (inline, no external dep)
auto trim = [](const string& str) -> string {
    size_t l = str.find_first_not_of(" \t\r\n");
    if (l == string::npos) return "";
    size_t r = str.find_last_not_of(" \t\r\n");
    return str.substr(l, r - l + 1);
};

TEST_CASE("REPL Trim and Tokenize", "[repl]") {
    // Trim
    REQUIRE(trim("  hello  \n") == "hello");
    REQUIRE(trim("\t\t\t") == "");
    REQUIRE(trim("") == "");

    // Tokenize
    auto tokens = Router::tokenize("  hello world  ");
    REQUIRE(tokens == vector<string>{"hello", "world"});
}

TEST_CASE("REPL Engine Commands", "[repl]") {
    Engine engine(".");  // Stub dir
    // Stub router with module
    auto stub = std::make_shared<StubModule>();
    engine.add_module(stub);

    // Help
    string help_out = engine.dispatch_command("help");
    REQUIRE(help_out.find("test_cmd") != string::npos);

    // Modules
    string mods_out = engine.dispatch_command("modules");
    REQUIRE(mods_out.find("stub") != string::npos);

    // Quests (stub—no file, empty)
    string quests_out = engine.dispatch_command("quests");
    REQUIRE(quests_out.find("Available quests") != string::npos);
}

TEST_CASE("Router Dispatch", "[router]") {
    Router router;
    auto stub = std::make_shared<StubModule>();
    router.add_module(stub);

    // Match
    string out = router.handle_input("test_cmd arg1");
    REQUIRE(out == "test output: 1");

    // No match
    out = router.handle_input("unknown");
    REQUIRE(out.empty());

    // List
    auto cmds = router.list_commands();
    REQUIRE(cmds == vector<string>{"test_cmd"});
}

TEST_CASE("Linux Module Load and Run", "[linux]") {
    LinuxModule mod;
    // Temp YAML file for test (in-memory, but for simplicity, write/read file)
    string yaml_str = R"(
        scenario: test
        scenarios:
          test:
            current_dir: /
            filesystem:
              /:
                test_file:
                  content: "test content"
                  perms: "rw-r--r--"
    )";
    ofstream temp_yaml("temp_test.yaml");
    temp_yaml << yaml_str;
    temp_yaml.close();

    // Load
    bool loaded = mod.load_from_path(".");
    REQUIRE(loaded == true);
    REQUIRE(mod.fs_size() == 1);  // 1 dir ( / with test_file)

    // Run 'ls'
    string ls_out = mod.run_command("ls", {});
    REQUIRE(ls_out.find("test_file (rw-r--r--) ") != string::npos);

    // Run 'cat'
    string cat_out = mod.run_command("cat", {"test_file"});
    REQUIRE(cat_out.find("test content") != string::npos);

    // Cleanup
    remove("temp_test.yaml");
}

TEST_CASE("Quest Activation and Eval", "[quests]") {
    Engine engine(".");
    auto stub = std::make_shared<StubModule>();
    engine.add_module(stub);

    // Activate
    string activate_out = engine.dispatch_command("quests stub 0");
    REQUIRE(activate_out.find("Activated quest 0 for stub") != string::npos);

    // Re-load/eval (simulate re-run)
    // Mock YAML for quest
    string yaml_str = R"(quests:
      - title: "Test Quest"
        condition:
          test: true)";
    Node yaml_node = YAML::Load(yaml_str);
    // Eval
    bool passed = stub->evaluate_condition(yaml_node["quests"][0]["condition"]);
    REQUIRE(passed == true);
}