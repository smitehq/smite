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

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: " #condition)); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' does not contain '" + substr + "'"); \
    }

int main() {
    std::cout << "========================================\n";
    std::cout << "Black Friday Quest Test\n";
    std::cout << "========================================\n";

    TEST("Load module and activate Black Friday quest")
        auto module = std::make_shared<KubernetesModule>();
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);

        bool activated = module->activate_quest("k8s_black_friday");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("Initial state - payment-processor is CrashLoopBackOff with 47 restarts")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";

        // Should see payment-processor with high restart count
        ASSERT_STR_CONTAINS(output, "payment-processor");
        ASSERT_STR_CONTAINS(output, "CrashLoopBackOff");
        ASSERT_STR_CONTAINS(output, "47");
    END_TEST()

    TEST("Initial state - frontend and product-api are Running but with restarts")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";

        // Frontend and product-api should be running but show restart counts
        ASSERT_STR_CONTAINS(output, "frontend");
        ASSERT_STR_CONTAINS(output, "product-api");
    END_TEST()

    TEST("Logs show payment-processor is OOMKilled under load")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        std::string output = module->run_command("kubectl logs", {"payment-processor-9k3d2c-zh8vx"});
        std::cout << "Output:\n" << output << "\n";

        ASSERT_STR_CONTAINS(output, "High transaction volume");
        ASSERT_STR_CONTAINS(output, "Queue depth");
        ASSERT_STR_CONTAINS(output, "OOMKilled");
    END_TEST()

    TEST("Describe payment-processor pod shows the problem")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        std::string output = module->run_command("kubectl describe pod", {"payment-processor-9k3d2c-zh8vx"});
        std::cout << "Output:\n" << output << "\n";

        ASSERT_STR_CONTAINS(output, "BackOff");
        ASSERT_STR_CONTAINS(output, "Killing");
    END_TEST()

    TEST("Frontend logs show payment API timeouts")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        std::string output = module->run_command("kubectl logs", {"frontend-7d9c8b-xk2mp"});
        std::cout << "Output:\n" << output << "\n";

        ASSERT_STR_CONTAINS(output, "Payment API timeout");
    END_TEST()

    TEST("Check deployments shows payment-processor has only 1 replica")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        std::string output = module->run_command("kubectl get deployments", {});
        std::cout << "Output:\n" << output << "\n";

        ASSERT_STR_CONTAINS(output, "payment-processor");
        ASSERT_STR_CONTAINS(output, "0/1"); // 0 ready out of 1 replica
    END_TEST()

    TEST("Quest condition NOT satisfied with 1 replica")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        YAML::Node condition;
        condition["type"] = "deployment_replicas";
        condition["deployment"] = "payment-processor";
        condition["min_replicas"] = 10;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("Simulate scaling payment-processor to 10 replicas")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        // Simulate the scale command
        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "payment-processor") {
                dep.replicas = 10;
                dep.ready_replicas = 10;
                dep.available_replicas = 10;
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_replicas";
        condition["deployment"] = "payment-processor";
        condition["min_replicas"] = 10;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    TEST("Quest condition satisfied with exactly 10 replicas")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "payment-processor") {
                dep.replicas = 10;
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_replicas";
        condition["deployment"] = "payment-processor";
        condition["min_replicas"] = 10;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    TEST("Quest condition satisfied with more than 10 replicas")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_black_friday");

        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "payment-processor") {
                dep.replicas = 15;
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_replicas";
        condition["deployment"] = "payment-processor";
        condition["min_replicas"] = 10;

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
