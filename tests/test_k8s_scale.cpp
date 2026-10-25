#include "../src/modules/kubernetes/module.h"
#include "../src/core/router.h"
#include "../src/core/utils.h"
#include <iostream>
#include <memory>
#include <string>

// Focused coverage for kubectl scale. The command is registered as the
// multi-word prefix "kubectl scale deployment" (alias: "kubectl scale deploy").
// Router::handle_command strips that prefix, so
// `kubectl scale deployment api --replicas=5` reaches the handler as
// {"api", "--replicas=5"} (args.size() == 2).

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "\n=== Test: " << name << " ===\n"; \
    try {

#define END_TEST() \
        tests_passed++; \
        std::cout << "PASSED\n"; \
    } catch (const std::exception& e) { \
        tests_failed++; \
        std::cout << "FAILED: " << e.what() << "\n"; \
    }

#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected: ") + std::to_string(expected) + ", Got: " + std::to_string(actual)); \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: " #condition)); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' does not contain '" + substr + "'"); \
    }

#define ASSERT_STR_NOT_CONTAINS(str, substr) \
    if ((str).find(substr) != std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' should not contain '" + substr + "'"); \
    }

std::shared_ptr<KubernetesModule> load_scale_module() {
    auto module = std::make_shared<KubernetesModule>();
    if (!module->load_from_path("src/modules/kubernetes")) {
        throw std::runtime_error("failed to load kubernetes module");
    }
    if (!module->activate_quest("k8s_deployment_scale")) {
        throw std::runtime_error("failed to activate k8s_deployment_scale");
    }
    return module;
}

// Same longest-prefix slice the router applies before calling the handler.
std::vector<std::string> args_after_prefix(const std::vector<std::string>& tokens,
                                           const std::string& prefix) {
    const auto prefix_tokens = Utils::tokenize_command(prefix);
    if (tokens.size() < prefix_tokens.size()) {
        throw std::runtime_error("command is shorter than registered prefix");
    }
    for (size_t i = 0; i < prefix_tokens.size(); ++i) {
        if (tokens[i] != prefix_tokens[i]) {
            throw std::runtime_error("command does not match registered prefix");
        }
    }
    return std::vector<std::string>(tokens.begin() + prefix_tokens.size(), tokens.end());
}

std::string route(const std::shared_ptr<KubernetesModule>& module, const std::string& cmd) {
    Router router;
    router.add_module(module);
    return router.handle_command(Utils::tokenize_command(cmd));
}

int replica_count(const std::shared_ptr<KubernetesModule>& module, const std::string& name) {
    for (const auto& dep : module->get_deployments()) {
        if (dep.name == name) return dep.replicas;
    }
    return -1;
}

int ready_count(const std::shared_ptr<KubernetesModule>& module, const std::string& name) {
    for (const auto& dep : module->get_deployments()) {
        if (dep.name == name) return dep.ready_replicas;
    }
    return -1;
}

int available_count(const std::shared_ptr<KubernetesModule>& module, const std::string& name) {
    for (const auto& dep : module->get_deployments()) {
        if (dep.name == name) return dep.available_replicas;
    }
    return -1;
}

int main() {
    std::cout << "========================================\n";
    std::cout << "kubectl scale argument routing tests\n";
    std::cout << "========================================\n";

    TEST("Registered prefix leaves two arguments for the reported command")
        auto module = load_scale_module();
        bool registered = false;
        for (const auto& prefix : module->registered_prefixes()) {
            if (prefix == "kubectl scale deployment") registered = true;
        }
        ASSERT_TRUE(registered);

        const std::string cmd = "kubectl scale deployment api --replicas=5";
        auto tokens = Utils::tokenize_command(cmd);
        auto args = args_after_prefix(tokens, "kubectl scale deployment");
        // This is the failure the quest hit: size 2 was rejected by `< 3`.
        ASSERT_EQ(args.size(), 2);
        ASSERT_TRUE(args[0] == "api");
        ASSERT_TRUE(args[1] == "--replicas=5");

        std::string output = module->run_command("kubectl scale deployment", args);
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "deployment.apps/api scaled");
        ASSERT_EQ(replica_count(module, "api"), 5);
        ASSERT_EQ(ready_count(module, "api"), 5);
        ASSERT_EQ(available_count(module, "api"), 5);
    END_TEST()

    TEST("Router dispatch of kubectl scale deployment api --replicas=5 scales api and completes the quest")
        auto module = load_scale_module();
        ASSERT_EQ(replica_count(module, "api"), 1);

        std::string output = route(module, "kubectl scale deployment api --replicas=5");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "deployment.apps/api scaled");
        ASSERT_STR_CONTAINS(output, "QUEST COMPLETE!");
        ASSERT_EQ(replica_count(module, "api"), 5);
        ASSERT_EQ(ready_count(module, "api"), 5);
        ASSERT_EQ(available_count(module, "api"), 5);

        std::string listing = route(module, "kubectl get deployments");
        std::cout << "Deployments:\n" << listing << "\n";
        ASSERT_STR_CONTAINS(listing, "api");
        ASSERT_STR_CONTAINS(listing, "5/5");
    END_TEST()

    TEST("deploy alias scales through the same two-argument remainder")
        auto module = load_scale_module();
        const std::string cmd = "kubectl scale deploy api --replicas=5";
        auto args = args_after_prefix(Utils::tokenize_command(cmd), "kubectl scale deploy");
        ASSERT_EQ(args.size(), 2);

        std::string output = route(module, cmd);
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "deployment.apps/api scaled");
        ASSERT_EQ(replica_count(module, "api"), 5);
    END_TEST()

    TEST("Space-separated --replicas flag scales the deployment")
        auto module = load_scale_module();
        const std::string cmd = "kubectl scale deployment api --replicas 5";
        auto args = args_after_prefix(Utils::tokenize_command(cmd), "kubectl scale deployment");
        ASSERT_EQ(args.size(), 3);
        ASSERT_TRUE(args[1] == "--replicas");
        ASSERT_TRUE(args[2] == "5");

        std::string output = route(module, cmd);
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "deployment.apps/api scaled");
        ASSERT_EQ(replica_count(module, "api"), 5);
    END_TEST()

    TEST("Scale below the quest minimum updates replicas without completing the quest")
        auto module = load_scale_module();
        std::string output = route(module, "kubectl scale deploy api --replicas 3");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "deployment.apps/api scaled");
        ASSERT_STR_NOT_CONTAINS(output, "QUEST COMPLETE!");
        ASSERT_EQ(replica_count(module, "api"), 3);
        ASSERT_EQ(ready_count(module, "api"), 3);
        ASSERT_EQ(available_count(module, "api"), 3);
    END_TEST()

    TEST("Missing name returns a usage error and leaves replicas unchanged")
        auto module = load_scale_module();
        std::string output = route(module, "kubectl scale deployment");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Error:");
        ASSERT_STR_CONTAINS(output, "deployment name");
        ASSERT_STR_CONTAINS(output, "Usage: kubectl scale deployment <name> --replicas=<count>");
        ASSERT_STR_NOT_CONTAINS(output, "scaled");
        ASSERT_EQ(replica_count(module, "api"), 1);
    END_TEST()

    TEST("Name without --replicas returns a usage error and leaves replicas unchanged")
        auto module = load_scale_module();
        std::string output = route(module, "kubectl scale deployment api");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Error:");
        ASSERT_STR_CONTAINS(output, "--replicas flag is required");
        ASSERT_STR_CONTAINS(output, "Usage: kubectl scale deployment <name> --replicas=<count>");
        ASSERT_STR_NOT_CONTAINS(output, "scaled");
        ASSERT_EQ(replica_count(module, "api"), 1);
    END_TEST()

    TEST("Invalid --replicas value returns an error and leaves replicas unchanged")
        auto module = load_scale_module();
        std::string output = route(module, "kubectl scale deployment api --replicas=abc");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Error: Invalid replicas value");
        ASSERT_STR_NOT_CONTAINS(output, "scaled");
        ASSERT_EQ(replica_count(module, "api"), 1);

        output = route(module, "kubectl scale deployment api --replicas");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Error: Invalid replicas value");
        ASSERT_EQ(replica_count(module, "api"), 1);

        output = route(module, "kubectl scale deployment api --replicas=-1");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Error: Invalid replicas value");
        ASSERT_EQ(replica_count(module, "api"), 1);
    END_TEST()

    TEST("Unknown deployment returns a not-found error")
        auto module = load_scale_module();
        std::string output = route(module, "kubectl scale deployment missing --replicas=5");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Error: deployment \"missing\" not found");
        ASSERT_STR_NOT_CONTAINS(output, "scaled");
        ASSERT_EQ(replica_count(module, "api"), 1);
    END_TEST()

    TEST("Scale to zero is accepted")
        auto module = load_scale_module();
        std::string output = route(module, "kubectl scale deployment api --replicas=0");
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "deployment.apps/api scaled");
        ASSERT_STR_NOT_CONTAINS(output, "QUEST COMPLETE!");
        ASSERT_EQ(replica_count(module, "api"), 0);
        ASSERT_EQ(ready_count(module, "api"), 0);
        ASSERT_EQ(available_count(module, "api"), 0);
    END_TEST()

    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << tests_passed << "\n";
    std::cout << "Tests Failed: " << tests_failed << "\n";

    if (tests_failed == 0) {
        std::cout << "\nALL TESTS PASSED\n\n";
        return 0;
    }

    std::cout << "\nSOME TESTS FAILED\n\n";
    return 1;
}
