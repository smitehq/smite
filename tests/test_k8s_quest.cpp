#include "../src/modules/kubernetes/module.h"
#include <iostream>
#include <cassert>
#include <string>

// Simple test framework
int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) \
    std::cout << "\n=== Test: " << name << " ===\n"; \
    try {

#define END_TEST() \
        tests_passed++; \
        std::cout << "✓ PASSED\n"; \
    } catch (const std::exception& e) { \
        tests_failed++; \
        std::cout << "✗ FAILED: " << e.what() << "\n"; \
    }

#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected: ") + std::to_string(expected) + ", Got: " + std::to_string(actual)); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' does not contain '" + substr + "'"); \
    }

#define ASSERT_STR_NOT_CONTAINS(str, substr) \
    if ((str).find(substr) != std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' should not contain '" + substr + "'"); \
    }

int main() {
    std::cout << "========================================\n";
    std::cout << "Kubernetes Quest Automated Test Suite\n";
    std::cout << "========================================\n";

    auto module = std::make_shared<KubernetesModule>();

    TEST("Module loads from path")
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);
    END_TEST()

    TEST("kubectl get pods shows backend in Running state before quest activation")
        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";
        // Should show default state with backend running
        // Check that the line with "backend" also contains "Running"
        bool backend_running = false;
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("backend") != std::string::npos && line.find("Running") != std::string::npos) {
                backend_running = true;
                break;
            }
        }
        if (!backend_running) {
            throw std::runtime_error("backend pod is not Running in default state");
        }
    END_TEST()

    TEST("Activate k8s_crashloop quest")
        bool activated = module->activate_quest("k8s_crashloop");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("kubectl get pods shows backend in CrashLoopBackOff after quest activation")
        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";
        // Check that the line with "backend" contains "CrashLoopBackOff"
        bool backend_crashing = false;
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("backend") != std::string::npos && line.find("CrashLoopBackOff") != std::string::npos) {
                backend_crashing = true;
                break;
            }
        }
        if (!backend_crashing) {
            throw std::runtime_error("backend pod is not in CrashLoopBackOff after quest activation");
        }
    END_TEST()

    TEST("kubectl describe pod backend shows secret error")
        std::string output = module->run_command("kubectl describe pod", {"backend"});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "db-secret");
        ASSERT_STR_CONTAINS(output, "not found");
    END_TEST()

    TEST("kubectl logs backend shows database connection error")
        std::string output = module->run_command("kubectl logs", {"backend"});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "database");
        ASSERT_STR_CONTAINS(output, "Error");
    END_TEST()

    TEST("kubectl get secrets shows frontend-secret but NOT db-secret")
        std::string output = module->run_command("kubectl get secrets", {});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "frontend-secret");
        ASSERT_STR_NOT_CONTAINS(output, "db-secret");
    END_TEST()

    TEST("kubectl create secret generic db-secret creates the secret and shows completion message")
        std::string output = module->run_command("kubectl create secret generic",
            {"db-secret", "--from-literal", "username=admin", "--from-literal", "password=supersecret"});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "secret/db-secret created");
        ASSERT_STR_CONTAINS(output, "QUEST COMPLETE!");
        // ASSERT_STR_CONTAINS(output, "YOU HAVE SMITED IT");
        // ASSERT_STR_CONTAINS(output, "Golden Kubeconfig");
    END_TEST()

    TEST("kubectl get secrets now shows db-secret")
        std::string output = module->run_command("kubectl get secrets", {});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "db-secret");
        ASSERT_STR_CONTAINS(output, "2");  // Should have 2 data items
    END_TEST()

    TEST("kubectl get pods shows backend is now Running (quest trigger worked!)")
        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";
        // Check that the line with "backend" contains "Running" and NOT "CrashLoopBackOff"
        bool backend_running = false;
        bool backend_crashing = false;
        std::istringstream iss(output);
        std::string line;
        while (std::getline(iss, line)) {
            if (line.find("backend") != std::string::npos) {
                if (line.find("Running") != std::string::npos) {
                    backend_running = true;
                }
                if (line.find("CrashLoopBackOff") != std::string::npos) {
                    backend_crashing = true;
                }
                break;
            }
        }
        if (!backend_running) {
            throw std::runtime_error("backend pod is not Running after creating secret");
        }
        if (backend_crashing) {
            throw std::runtime_error("backend pod is still in CrashLoopBackOff after creating secret");
        }
    END_TEST()

    TEST("kubectl logs backend shows successful database connection")
        std::string output = module->run_command("kubectl logs", {"backend"});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Connecting to database");
        ASSERT_STR_CONTAINS(output, "Database connection established");
        ASSERT_STR_CONTAINS(output, "Backend service started successfully");
    END_TEST()

    TEST("kubectl describe pod backend shows Started event")
        std::string output = module->run_command("kubectl describe pod", {"backend"});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "Normal");
        ASSERT_STR_CONTAINS(output, "Started");
    END_TEST()

    TEST("Quest condition is satisfied (pod is Running)")
        YAML::Node condition;
        condition["type"] = "pod_status";
        condition["pod"] = "backend";
        condition["status"] = "Running";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    // Print summary
    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << tests_passed << "\n";
    std::cout << "Tests Failed: " << tests_failed << "\n";

    if (tests_failed == 0) {
        std::cout << "\n🎉 ALL TESTS PASSED! 🎉\n\n";
        return 0;
    } else {
        std::cout << "\n❌ SOME TESTS FAILED ❌\n\n";
        return 1;
    }
}
