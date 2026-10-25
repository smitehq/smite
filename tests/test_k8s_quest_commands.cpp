#include "../src/modules/kubernetes/module.h"
#include "../src/core/router.h"
#include "../src/core/utils.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <unistd.h>

namespace fs = std::filesystem;

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

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Assertion failed: ") + #condition); \
    }

#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected: ") + std::to_string(expected) + ", Got: " + std::to_string(actual)); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + (str) + "' does not contain '" + (substr) + "'"); \
    }

#define ASSERT_STR_NOT_CONTAINS(str, substr) \
    if ((str).find(substr) != std::string::npos) { \
        throw std::runtime_error(std::string("String '") + (str) + "' should not contain '" + (substr) + "'"); \
    }

std::shared_ptr<KubernetesModule> load_quest(const std::string& quest_id) {
    auto module = std::make_shared<KubernetesModule>();
    if (!module->load_from_path("src/modules/kubernetes")) {
        throw std::runtime_error("failed to load kubernetes module");
    }
    if (!module->activate_quest(quest_id)) {
        throw std::runtime_error("failed to activate " + quest_id);
    }
    return module;
}

std::string route(const std::shared_ptr<KubernetesModule>& module, const std::string& cmd) {
    Router router;
    router.add_module(module);
    return router.handle_command(Utils::tokenize_command(cmd));
}

struct TempFile {
    fs::path path;
    TempFile(fs::path file, const std::string& content) : path(std::move(file)) {
        std::ofstream out(path);
        if (!out) throw std::runtime_error("failed to write " + path.string());
        out << content;
    }
    ~TempFile() { std::error_code ec; fs::remove(path, ec); }
};

struct ScopedEnv {
    std::string name;
    ScopedEnv(std::string key, const std::string& value) : name(std::move(key)) {
        setenv(name.c_str(), value.c_str(), 1);
    }
    ~ScopedEnv() { unsetenv(name.c_str()); }
};

int main() {
    std::cout << "========================================\n";
    std::cout << "Quest kubectl command routing tests\n";
    std::cout << "========================================\n";

    TEST("kubectl rollout status|history|undo deployment <name> rolls web-app back")
        auto module = load_quest("k8s_rolling_update");

        std::string status = route(module, "kubectl rollout status deployment web-app");
        std::cout << status;
        ASSERT_STR_CONTAINS(status, "Waiting for deployment \"web-app\"");
        ASSERT_STR_NOT_CONTAINS(status, "not found");
        ASSERT_STR_NOT_CONTAINS(status, "not implemented");

        std::string history = route(module, "kubectl rollout history deployment web-app");
        std::cout << history;
        ASSERT_STR_CONTAINS(history, "deployment.apps/web-app");
        ASSERT_STR_CONTAINS(history, "REVISION");
        ASSERT_STR_CONTAINS(history, "3");

        std::string undo = route(module, "kubectl rollout undo deployment web-app");
        std::cout << undo;
        ASSERT_STR_CONTAINS(undo, "deployment.apps/web-app rolled back");
        ASSERT_STR_CONTAINS(undo, "QUEST COMPLETE!");

        std::string after = route(module, "kubectl rollout status deployment/web-app");
        std::cout << after;
        ASSERT_STR_CONTAINS(after, "successfully rolled out");
    END_TEST()

    TEST("kubectl rollout undo deployment payment-service clears the broken image")
        auto module = load_quest("k8s_rollout_disaster");
        std::string undo = route(module, "kubectl rollout undo deployment payment-service");
        std::cout << undo;
        ASSERT_STR_CONTAINS(undo, "deployment.apps/payment-service rolled back");
        ASSERT_STR_CONTAINS(undo, "QUEST COMPLETE!");

        std::string pods = route(module, "kubectl get pods");
        std::cout << pods;
        ASSERT_STR_CONTAINS(pods, "Running");
        ASSERT_STR_NOT_CONTAINS(pods, "broken");
        ASSERT_STR_NOT_CONTAINS(pods, "CrashLoopBackOff");
    END_TEST()

    TEST("rollout without a deployment name is an error")
        auto module = load_quest("k8s_rolling_update");
        std::string output = route(module, "kubectl rollout undo deployment");
        std::cout << output;
        ASSERT_STR_CONTAINS(output, "Error:");
        ASSERT_STR_CONTAINS(output, "resource name required");
        ASSERT_STR_NOT_CONTAINS(output, "QUEST COMPLETE!");
    END_TEST()

    TEST("kubectl get statefulsets and scale statefulset cassandra --replicas=3")
        auto module = load_quest("k8s_statefulset_order");
        std::string before = route(module, "kubectl get statefulsets");
        std::cout << before;
        ASSERT_STR_CONTAINS(before, "cassandra");
        ASSERT_STR_CONTAINS(before, "1/1");

        std::string alias = route(module, "kubectl get sts");
        ASSERT_STR_CONTAINS(alias, "cassandra");

        std::string watching = route(module, "kubectl get pods -w");
        ASSERT_STR_CONTAINS(watching, "cassandra-0");
        ASSERT_STR_NOT_CONTAINS(watching, "not implemented");

        std::string scaled = route(module, "kubectl scale statefulset cassandra --replicas=3");
        std::cout << scaled;
        ASSERT_STR_CONTAINS(scaled, "statefulset.apps/cassandra scaled");
        ASSERT_STR_CONTAINS(scaled, "QUEST COMPLETE!");

        std::string after = route(module, "kubectl get statefulset cassandra");
        ASSERT_STR_CONTAINS(after, "3/3");
        std::string pods = route(module, "kubectl get pods");
        ASSERT_STR_CONTAINS(pods, "cassandra-1");
        ASSERT_STR_CONTAINS(pods, "cassandra-2");
    END_TEST()

    TEST("scale sts alias and missing replicas leave the statefulset unchanged")
        auto module = load_quest("k8s_statefulset_order");
        std::string missing = route(module, "kubectl scale sts cassandra");
        ASSERT_STR_CONTAINS(missing, "--replicas flag is required");
        ASSERT_STR_NOT_CONTAINS(missing, "scaled");

        std::string scaled = route(module, "kubectl scale sts cassandra --replicas 3");
        ASSERT_STR_CONTAINS(scaled, "statefulset.apps/cassandra scaled");
        ASSERT_STR_CONTAINS(scaled, "QUEST COMPLETE!");
    END_TEST()

    TEST("kubectl get resourcequota and delete deployment old-test")
        auto module = load_quest("k8s_resource_quota");
        std::string quota = route(module, "kubectl get resourcequota");
        std::cout << quota;
        ASSERT_STR_CONTAINS(quota, "pods-medium");
        ASSERT_STR_CONTAINS(quota, "pods: 10/10");

        std::string deleted = route(module, "kubectl delete deployment old-test");
        std::cout << deleted;
        ASSERT_STR_CONTAINS(deleted, "deployment.apps \"old-test\" deleted");
        ASSERT_STR_CONTAINS(deleted, "QUEST COMPLETE!");

        std::string deployments = route(module, "kubectl get deployments");
        ASSERT_STR_NOT_CONTAINS(deployments, "old-test");
    END_TEST()

    TEST("delete deployment reports a missing name and an unknown deployment")
        auto module = load_quest("k8s_resource_quota");
        std::string usage = route(module, "kubectl delete deployment");
        ASSERT_STR_CONTAINS(usage, "deployment name required");
        std::string missing = route(module, "kubectl delete deploy missing");
        ASSERT_STR_CONTAINS(missing, "not found");
        ASSERT_STR_NOT_CONTAINS(missing, "QUEST COMPLETE!");
    END_TEST()

    TEST("kubectl get pvc and get pv, then create -f binds a matching volume")
        auto module = load_quest("k8s_persistent_storage");
        std::string pvcs = route(module, "kubectl get pvc");
        std::cout << pvcs;
        ASSERT_STR_CONTAINS(pvcs, "database-pvc");
        ASSERT_STR_CONTAINS(pvcs, "Pending");
        ASSERT_STR_CONTAINS(pvcs, "10Gi");

        std::string pvs = route(module, "kubectl get pv");
        std::cout << pvs;
        ASSERT_STR_CONTAINS(pvs, "small-fast-pv");
        ASSERT_STR_CONTAINS(pvs, "1Gi");
        ASSERT_STR_CONTAINS(pvs, "Available");

        TempFile manifest("database-pv.yaml",
            "apiVersion: v1\n"
            "kind: PersistentVolume\n"
            "metadata:\n"
            "  name: database-pv\n"
            "spec:\n"
            "  capacity:\n"
            "    storage: 10Gi\n"
            "  accessModes:\n"
            "  - ReadWriteOnce\n"
            "  storageClassName: standard\n");

        std::string created = route(module, "kubectl create -f database-pv.yaml");
        std::cout << created;
        ASSERT_STR_CONTAINS(created, "persistentvolume/database-pv created");
        ASSERT_STR_CONTAINS(created, "bound to database-pvc");
        ASSERT_STR_CONTAINS(created, "QUEST COMPLETE!");

        std::string after = route(module, "kubectl get pvc");
        ASSERT_STR_CONTAINS(after, "Bound");
        ASSERT_STR_CONTAINS(after, "database-pv");
    END_TEST()

    TEST("kubectl get networkpolicies and create -f networkpolicy.yaml")
        auto module = load_quest("k8s_network_policy");
        std::string before = route(module, "kubectl get networkpolicies");
        std::cout << before;
        ASSERT_STR_CONTAINS(before, "No resources found");

        std::string netpol = route(module, "kubectl get netpol");
        ASSERT_STR_CONTAINS(netpol, "No resources found");

        TempFile manifest("networkpolicy.yaml",
            "apiVersion: networking.k8s.io/v1\n"
            "kind: NetworkPolicy\n"
            "metadata:\n"
            "  name: database-firewall\n"
            "spec:\n"
            "  podSelector:\n"
            "    matchLabels:\n"
            "      app: database\n"
            "  policyTypes:\n"
            "  - Ingress\n");

        std::string created = route(module, "kubectl create -f networkpolicy.yaml");
        std::cout << created;
        ASSERT_STR_CONTAINS(created, "networkpolicy.networking.k8s.io/database-firewall created");
        ASSERT_STR_CONTAINS(created, "QUEST COMPLETE!");

        std::string after = route(module, "kubectl get networkpolicy");
        ASSERT_STR_CONTAINS(after, "database-firewall");
    END_TEST()

    TEST("kubectl create -f reports a missing file")
        auto module = load_quest("k8s_network_policy");
        std::string output = route(module, "kubectl create -f does-not-exist.yaml");
        ASSERT_STR_CONTAINS(output, "does not exist");
        ASSERT_STR_NOT_CONTAINS(output, "QUEST COMPLETE!");
    END_TEST()

    TEST("kubectl cordon worker-2 marks the node unschedulable")
        auto module = load_quest("k8s_node_drain");
        std::string cordoned = route(module, "kubectl cordon worker-2");
        std::cout << cordoned;
        ASSERT_STR_CONTAINS(cordoned, "node/worker-2 cordoned");
        ASSERT_STR_CONTAINS(cordoned, "QUEST COMPLETE!");

        std::string nodes = route(module, "kubectl get nodes");
        std::cout << nodes;
        ASSERT_STR_CONTAINS(nodes, "worker-2");
        ASSERT_STR_CONTAINS(nodes, "Ready,SchedulingDisabled");
    END_TEST()

    TEST("kubectl drain worker-2 --ignore-daemonsets cordons the node and moves pods")
        auto module = load_quest("k8s_node_drain");
        std::string drained = route(module, "kubectl drain worker-2 --ignore-daemonsets");
        std::cout << drained;
        ASSERT_STR_CONTAINS(drained, "node/worker-2 cordoned");
        ASSERT_STR_CONTAINS(drained, "evicting pod database-primary");
        ASSERT_STR_CONTAINS(drained, "node/worker-2 drained");
        ASSERT_STR_CONTAINS(drained, "QUEST COMPLETE!");

        bool still_there = false;
        bool moved = false;
        for (const auto& node : module->get_nodes()) {
            for (const auto& pod : node.pods) {
                if (pod.name != "database-primary") continue;
                if (node.name == "worker-2") still_there = true;
                else moved = true;
            }
        }
        ASSERT_TRUE(moved);
        ASSERT_TRUE(!still_there);

        std::string wide = route(module, "kubectl get pods -o wide");
        ASSERT_STR_CONTAINS(wide, "database-primary");
        ASSERT_STR_CONTAINS(wide, "worker-1");
    END_TEST()

    TEST("kubectl describe nodes shows capacity for every node")
        auto module = load_quest("k8s_resource_request");
        std::string described = route(module, "kubectl describe nodes");
        std::cout << described;
        ASSERT_STR_CONTAINS(described, "worker-1");
        ASSERT_STR_CONTAINS(described, "worker-2");
        ASSERT_STR_CONTAINS(described, "Capacity:");
        ASSERT_STR_CONTAINS(described, "memory: 8Gi");
        ASSERT_STR_NOT_CONTAINS(described, "not implemented");
    END_TEST()

    TEST("kubectl get pods --show-labels prints pod labels")
        auto module = load_quest("k8s_service_discovery");
        std::string pods = route(module, "kubectl get pods --show-labels");
        std::cout << pods;
        ASSERT_STR_CONTAINS(pods, "LABELS");
        ASSERT_STR_CONTAINS(pods, "app=api-server");
        ASSERT_STR_CONTAINS(pods, "version=v1");
        ASSERT_STR_NOT_CONTAINS(pods, "not implemented");
    END_TEST()

    TEST("kubectl config context commands then scale payment-service")
        auto module = load_quest("k8s_multi_cluster");
        std::string current = route(module, "kubectl config current-context");
        std::cout << current;
        ASSERT_STR_CONTAINS(current, "staging");

        std::string contexts = route(module, "kubectl config get-contexts");
        std::cout << contexts;
        ASSERT_STR_CONTAINS(contexts, "CURRENT");
        ASSERT_STR_CONTAINS(contexts, "*");
        ASSERT_STR_CONTAINS(contexts, "staging");
        ASSERT_STR_CONTAINS(contexts, "prod-us-east");
        ASSERT_STR_CONTAINS(contexts, "prod-eu-west");

        std::string switched = route(module, "kubectl config use-context prod-us-east");
        std::cout << switched;
        ASSERT_STR_CONTAINS(switched, "Switched to context \"prod-us-east\".");

        std::string now = route(module, "kubectl config current-context");
        ASSERT_STR_CONTAINS(now, "prod-us-east");

        std::string deployment = route(module, "kubectl get deployment payment-service");
        std::cout << deployment;
        ASSERT_STR_CONTAINS(deployment, "payment-service");
        ASSERT_STR_CONTAINS(deployment, "10/10");

        std::string scaled = route(module, "kubectl scale deployment payment-service --replicas=3");
        std::cout << scaled;
        ASSERT_STR_CONTAINS(scaled, "deployment.apps/payment-service scaled");
        ASSERT_STR_CONTAINS(scaled, "QUEST COMPLETE!");
    END_TEST()

    TEST("kubectl config use-context rejects an unknown context")
        auto module = load_quest("k8s_multi_cluster");
        std::string output = route(module, "kubectl config use-context no-such-cluster");
        ASSERT_STR_CONTAINS(output, "no context exists");
    END_TEST()

    TEST("kubectl get ingress -o yaml, describe ingress, and edit ingress fix routing")
        auto module = load_quest("k8s_ingress_routing");
        std::string yaml = route(module, "kubectl get ingress main-ingress -o yaml");
        std::cout << yaml;
        ASSERT_STR_CONTAINS(yaml, "kind: Ingress");
        ASSERT_STR_CONTAINS(yaml, "name: admin-panel");
        ASSERT_STR_CONTAINS(yaml, "host: example.com");
        ASSERT_STR_NOT_CONTAINS(yaml, "not implemented");

        std::string described = route(module, "kubectl describe ingress main-ingress");
        std::cout << described;
        ASSERT_STR_CONTAINS(described, "example.com");
        ASSERT_STR_CONTAINS(described, "admin-panel:3000");

        const char* fixed =
            "apiVersion: networking.k8s.io/v1\n"
            "kind: Ingress\n"
            "metadata:\n"
            "  name: main-ingress\n"
            "spec:\n"
            "  rules:\n"
            "  - host: example.com\n"
            "    http:\n"
            "      paths:\n"
            "      - path: /\n"
            "        pathType: Prefix\n"
            "        backend:\n"
            "          service:\n"
            "            name: web-app\n"
            "            port:\n"
            "              number: 80\n"
            "      - path: /api\n"
            "        pathType: Prefix\n"
            "        backend:\n"
            "          service:\n"
            "            name: api-service\n"
            "            port:\n"
            "              number: 8080\n"
            "  - host: admin.example.com\n"
            "    http:\n"
            "      paths:\n"
            "      - path: /\n"
            "        pathType: Prefix\n"
            "        backend:\n"
            "          service:\n"
            "            name: admin-panel\n"
            "            port:\n"
            "              number: 3000\n";

        std::string edited;
        {
            ScopedEnv editor("SMITE_EDIT_CONTENT", fixed);
            edited = route(module, "kubectl edit ingress main-ingress");
        }
        std::cout << edited;
        ASSERT_STR_CONTAINS(edited, "ingress.networking.k8s.io/main-ingress edited");
        ASSERT_STR_CONTAINS(edited, "QUEST COMPLETE!");

        std::string after = route(module, "kubectl describe ingress main-ingress");
        ASSERT_STR_CONTAINS(after, "web-app:80");
        ASSERT_STR_CONTAINS(after, "api-service:8080");
    END_TEST()

    TEST("kubectl label node worker-1 key=value records the label")
        auto module = load_quest("k8s_affinity_failure");
        std::string labeled = route(module, "kubectl label node worker-1 disktype=ssd");
        std::cout << labeled;
        ASSERT_STR_CONTAINS(labeled, "node/worker-1 labeled");
        ASSERT_STR_NOT_CONTAINS(labeled, "QUEST COMPLETE!");
        ASSERT_STR_NOT_CONTAINS(labeled, "not implemented");

        std::string described = route(module, "kubectl describe node worker-1");
        ASSERT_STR_CONTAINS(described, "disktype=ssd");

        std::string missing = route(module, "kubectl label node missing-node app=cache");
        ASSERT_STR_CONTAINS(missing, "not found");
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
