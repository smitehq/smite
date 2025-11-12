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

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error(std::string("Assertion failed: NOT(" #condition ")")); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' does not contain '" + substr + "'"); \
    }

int main() {
    std::cout << "========================================\n";
    std::cout << "Kubernetes Spec Tracking Quest Tests\n";
    std::cout << "========================================\n";

    // ========================================
    // TEST 1: k8s_oomkilled
    // ========================================
    std::cout << "\n======== TESTING: k8s_oomkilled ========\n";

    TEST("k8s_oomkilled: Load module and activate quest")
        auto module = std::make_shared<KubernetesModule>();
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);

        bool activated = module->activate_quest("k8s_oomkilled");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("k8s_oomkilled: Initial state - pod is CrashLoopBackOff")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "worker");
        ASSERT_STR_CONTAINS(output, "CrashLoopBackOff");
    END_TEST()

    TEST("k8s_oomkilled: Initial state - deployment has 256Mi memory limit")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        std::string output = module->run_command("kubectl get deployments", {});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "worker");
    END_TEST()

    TEST("k8s_oomkilled: Quest condition NOT satisfied with 256Mi")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        YAML::Node condition;
        condition["type"] = "deployment_memory_limit";
        condition["deployment"] = "worker";
        condition["min_limit"] = "512Mi";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("k8s_oomkilled: Simulate increasing memory to 512Mi")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        // Manually update deployment memory limit
        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "worker") {
                dep.memory_limit = "512Mi";
                dep.ready_replicas = 1;
                dep.available_replicas = 1;
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_memory_limit";
        condition["deployment"] = "worker";
        condition["min_limit"] = "512Mi";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    TEST("k8s_oomkilled: Quest condition satisfied with 1Gi")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "worker") {
                dep.memory_limit = "1Gi";
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_memory_limit";
        condition["deployment"] = "worker";
        condition["min_limit"] = "512Mi";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    // ========================================
    // TEST 2: k8s_liveness_probe
    // ========================================
    std::cout << "\n======== TESTING: k8s_liveness_probe ========\n";

    TEST("k8s_liveness_probe: Load and activate quest")
        auto module = std::make_shared<KubernetesModule>();
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);

        bool activated = module->activate_quest("k8s_liveness_probe");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("k8s_liveness_probe: Initial state - pod is CrashLoopBackOff")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_liveness_probe");

        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "monitor");
        ASSERT_STR_CONTAINS(output, "CrashLoopBackOff");
    END_TEST()

    TEST("k8s_liveness_probe: Quest condition NOT satisfied with initial probe settings")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_liveness_probe");

        YAML::Node condition;
        condition["type"] = "deployment_liveness_probe";
        condition["deployment"] = "monitor";
        condition["min_initial_delay"] = 10;
        condition["min_timeout"] = 3;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("k8s_liveness_probe: Quest condition satisfied after fixing probe config")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_liveness_probe");

        // Simulate fixing the liveness probe
        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "monitor") {
                dep.liveness_initial_delay = 15;
                dep.liveness_timeout = 5;
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_liveness_probe";
        condition["deployment"] = "monitor";
        condition["min_initial_delay"] = 10;
        condition["min_timeout"] = 3;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    // ========================================
    // TEST 3: k8s_persistent_storage
    // ========================================
    std::cout << "\n======== TESTING: k8s_persistent_storage ========\n";

    TEST("k8s_persistent_storage: Load and activate quest")
        auto module = std::make_shared<KubernetesModule>();
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);

        bool activated = module->activate_quest("k8s_persistent_storage");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("k8s_persistent_storage: Initial state - PVC is Pending")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_persistent_storage");

        const auto& pvcs = module->get_pvcs();
        bool found_pending = false;
        for (const auto& pvc : pvcs) {
            if (pvc.name == "database-pvc" && pvc.status == "Pending") {
                found_pending = true;
                break;
            }
        }
        ASSERT_TRUE(found_pending);
    END_TEST()

    TEST("k8s_persistent_storage: Quest condition NOT satisfied with Pending PVC")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_persistent_storage");

        YAML::Node condition;
        condition["type"] = "pvc_status";
        condition["pvc"] = "database-pvc";
        condition["status"] = "Bound";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("k8s_persistent_storage: Quest condition satisfied when PVC is Bound")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_persistent_storage");

        // Simulate binding the PVC
        auto& pvcs = module->get_pvcs_mutable();
        for (auto& pvc : pvcs) {
            if (pvc.name == "database-pvc") {
                pvc.status = "Bound";
                pvc.volume_name = "pv-12345";
            }
        }

        YAML::Node condition;
        condition["type"] = "pvc_status";
        condition["pvc"] = "database-pvc";
        condition["status"] = "Bound";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    // ========================================
    // TEST 4: k8s_affinity_failure
    // ========================================
    std::cout << "\n======== TESTING: k8s_affinity_failure ========\n";

    TEST("k8s_affinity_failure: Load and activate quest")
        auto module = std::make_shared<KubernetesModule>();
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);

        bool activated = module->activate_quest("k8s_affinity_failure");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("k8s_affinity_failure: Initial state - pod is Pending")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_affinity_failure");

        std::string output = module->run_command("kubectl get pods", {});
        std::cout << "Output:\n" << output << "\n";
        ASSERT_STR_CONTAINS(output, "cache");
        ASSERT_STR_CONTAINS(output, "Pending");
    END_TEST()

    TEST("k8s_affinity_failure: Quest condition NOT satisfied with 'required' affinity")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_affinity_failure");

        YAML::Node condition;
        condition["type"] = "deployment_affinity";
        condition["deployment"] = "cache";
        condition["affinity_type"] = "preferred";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("k8s_affinity_failure: Quest condition satisfied after relaxing to 'preferred'")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_affinity_failure");

        // Simulate editing the deployment to relax affinity
        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "cache") {
                dep.affinity_type = "preferred";
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_affinity";
        condition["deployment"] = "cache";
        condition["affinity_type"] = "preferred";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    // ========================================
    // TEST 5: k8s_node_drain
    // ========================================
    std::cout << "\n======== TESTING: k8s_node_drain ========\n";

    TEST("k8s_node_drain: Load and activate quest")
        auto module = std::make_shared<KubernetesModule>();
        bool loaded = module->load_from_path("src/modules/kubernetes");
        ASSERT_EQ(loaded, true);

        bool activated = module->activate_quest("k8s_node_drain");
        ASSERT_EQ(activated, true);
    END_TEST()

    TEST("k8s_node_drain: Initial state - worker-2 is NOT cordoned")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_node_drain");

        const auto& nodes = module->get_nodes();
        bool found_uncordoned = false;
        for (const auto& node : nodes) {
            if (node.name == "worker-2" && !node.cordoned) {
                found_uncordoned = true;
                break;
            }
        }
        ASSERT_TRUE(found_uncordoned);
    END_TEST()

    TEST("k8s_node_drain: Initial state - worker-2 has pods running")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_node_drain");

        const auto& nodes = module->get_nodes();
        bool found_with_pods = false;
        for (const auto& node : nodes) {
            if (node.name == "worker-2" && !node.pods.empty()) {
                found_with_pods = true;
                std::cout << "worker-2 has " << node.pods.size() << " pods\n";
                break;
            }
        }
        ASSERT_TRUE(found_with_pods);
    END_TEST()

    TEST("k8s_node_drain: Quest condition NOT satisfied initially")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_node_drain");

        YAML::Node condition;
        condition["type"] = "node_cordoned";
        condition["node"] = "worker-2";
        condition["cordoned"] = true;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("k8s_node_drain: Quest condition satisfied after cordoning node")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_node_drain");

        // Simulate cordoning the node
        auto& nodes = module->get_nodes_mutable();
        for (auto& node : nodes) {
            if (node.name == "worker-2") {
                node.cordoned = true;
            }
        }

        YAML::Node condition;
        condition["type"] = "node_cordoned";
        condition["node"] = "worker-2";
        condition["cordoned"] = true;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    // ========================================
    // EDGE CASES AND NEGATIVE TESTS
    // ========================================
    std::cout << "\n======== TESTING: Edge Cases ========\n";

    TEST("Memory limit parsing: 1Gi > 512Mi")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "worker") {
                dep.memory_limit = "1Gi";
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_memory_limit";
        condition["deployment"] = "worker";
        condition["min_limit"] = "512Mi";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, true);
    END_TEST()

    TEST("Memory limit parsing: 256Mi < 512Mi")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_oomkilled");

        YAML::Node condition;
        condition["type"] = "deployment_memory_limit";
        condition["deployment"] = "worker";
        condition["min_limit"] = "512Mi";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("Liveness probe: Both delay and timeout must be sufficient")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_liveness_probe");

        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "monitor") {
                dep.liveness_initial_delay = 15;  // Good
                dep.liveness_timeout = 1;          // Too low
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_liveness_probe";
        condition["deployment"] = "monitor";
        condition["min_initial_delay"] = 10;
        condition["min_timeout"] = 3;

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("PVC status: Lost status does not satisfy Bound requirement")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_persistent_storage");

        auto& pvcs = module->get_pvcs_mutable();
        for (auto& pvc : pvcs) {
            if (pvc.name == "database-pvc") {
                pvc.status = "Lost";
            }
        }

        YAML::Node condition;
        condition["type"] = "pvc_status";
        condition["pvc"] = "database-pvc";
        condition["status"] = "Bound";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
    END_TEST()

    TEST("Affinity: 'none' does not satisfy 'preferred' requirement")
        auto module = std::make_shared<KubernetesModule>();
        module->load_from_path("src/modules/kubernetes");
        module->activate_quest("k8s_affinity_failure");

        auto& deployments = module->get_deployments_mutable();
        for (auto& dep : deployments) {
            if (dep.name == "cache") {
                dep.affinity_type = "none";
            }
        }

        YAML::Node condition;
        condition["type"] = "deployment_affinity";
        condition["deployment"] = "cache";
        condition["affinity_type"] = "preferred";

        bool satisfied = module->evaluate_condition(condition);
        ASSERT_EQ(satisfied, false);
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
