#include "../src/core/engine.h"
#include "../src/core/paths.h"
#include "../src/core/router.h"
#include "../src/core/utils.h"
#include "../src/modules/kubernetes/module.h"
#include "../src/state/quest.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <set>
#include <string>
#include <unistd.h>
#include <vector>

namespace fs = std::filesystem;

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

#define ASSERT_TRUE(cond) \
    if (!(cond)) { \
        throw std::runtime_error(std::string("Condition failed: ") + #cond); \
    }

#define ASSERT_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected '") + (expected) + "', got '" + (actual) + "'"); \
    }

namespace {

const std::set<std::string> kProseWords = {
    "commands", "command", "client", "blade", "like", "itself"
};

const char* kHiddenQuests[] = {
    "k8s_affinity_failure",
    "k8s_ingress_routing",
    "k8s_multi_cluster",
    "k8s_network_policy",
    "k8s_node_drain",
    "k8s_persistent_storage",
    "k8s_resource_quota",
    "k8s_resource_request",
    "k8s_statefulset_order",
};

std::string trim_copy(std::string text) {
    return Utils::trim(text);
}

std::vector<std::string> extract_kubectl_commands(const std::string& text) {
    std::vector<std::string> commands;
    const std::string key = "kubectl";
    for (size_t pos = 0; pos < text.size();) {
        size_t found = text.find(key, pos);
        if (found == std::string::npos) break;

        bool boundary = found == 0 ||
            (!std::isalnum(static_cast<unsigned char>(text[found - 1])) &&
             text[found - 1] != '_' && text[found - 1] != '-');
        if (!boundary) {
            pos = found + key.size();
            continue;
        }

        size_t end = found + key.size();
        if (found > 0 && (text[found - 1] == '\'' || text[found - 1] == '"')) {
            char quote = text[found - 1];
            end = text.find(quote, found);
            if (end == std::string::npos) end = text.size();
        } else {
            while (end < text.size()) {
                char c = text[end];
                if (c == '\n' || c == '\r' || c == '\'' || c == '"' || c == '`' || c == ')') break;
                ++end;
            }
        }

        std::string cmd = trim_copy(text.substr(found, end - found));
        while (!cmd.empty() &&
               (cmd.back() == '.' || cmd.back() == ',' || cmd.back() == ';' ||
                cmd.back() == ':' || cmd.back() == '!')) {
            cmd.pop_back();
        }
        cmd = trim_copy(cmd);

        auto tokens = Utils::tokenize_command(cmd);
        if (tokens.size() >= 2 && tokens[0] == "kubectl") {
            const std::string& verb = tokens[1];
            bool verb_like = !verb.empty() &&
                std::islower(static_cast<unsigned char>(verb[0]));
            for (char c : verb) {
                if (!std::islower(static_cast<unsigned char>(c)) && c != '-') verb_like = false;
            }
            if (verb_like && !kProseWords.count(verb)) {
                commands.push_back(cmd);
            }
        }

        pos = std::max(end, found + key.size());
    }
    return commands;
}

bool kubectl_invocation_supported(const std::vector<std::string>& prefixes, const std::string& invocation) {
    auto tokens = Utils::tokenize_command(invocation);
    if (tokens.size() < 2 || tokens[0] != "kubectl") return true;

    size_t best = 0;
    for (const auto& prefix : prefixes) {
        // The bare "kubectl" prefix is the honest-error fallback, not an implementation.
        if (prefix == "kubectl") continue;
        auto prefix_tokens = Utils::tokenize_command(prefix);
        if (prefix_tokens.empty() || prefix_tokens.size() > tokens.size()) continue;
        bool match = true;
        for (size_t i = 0; i < prefix_tokens.size(); ++i) {
            if (prefix_tokens[i] != tokens[i]) {
                match = false;
                break;
            }
        }
        if (match && prefix_tokens.size() > best) best = prefix_tokens.size();
    }
    return best >= 2;
}

} // namespace

int main() {
    fs::path home = fs::temp_directory_path() / ("smite-first-run-" + std::to_string(getpid()));
    fs::remove_all(home);
    fs::create_directories(home);
    setenv("HOME", home.c_str(), 1);
    unsetenv("SMITE_DATA");

    std::cout << "========================================\n";
    std::cout << "First-run UX Test Suite\n";
    std::cout << "========================================\n";

TEST("SMITE_DATA overrides the module directory, otherwise it is found from the binary")
    setenv("SMITE_DATA", "/tmp/not-a-real-smite-dir", 1);
    auto overridden = smite::paths::modules_data_dir();
    unsetenv("SMITE_DATA");
    ASSERT_EQ(overridden.string(), std::string("/tmp/not-a-real-smite-dir"));

    auto discovered = smite::paths::modules_data_dir();
    auto quest_file = discovered / "kubernetes" / "quests" / "k8s_first_steps.yaml";
    if (!fs::exists(quest_file)) {
        throw std::runtime_error(
            "modules dir '" + discovered.string() +
            "' (exe " + smite::paths::executable_path().string() +
            ", cwd " + fs::current_path().string() + ") is missing " + quest_file.string());
    }
    auto state_dir = smite::paths::user_state_dir();
    ASSERT_EQ(state_dir.string(), (home / ".smite").string());
END_TEST()

TEST("Quests are sorted by difficulty, then name")
    QuestManager quests("src/modules");
    quests.load_all_quests();
    auto loaded = quests.get_quests_for_module("kubernetes");
    ASSERT_TRUE(loaded.size() > 1);
    for (size_t i = 1; i < loaded.size(); ++i) {
        const Quest& prev = loaded[i - 1];
        const Quest& cur = loaded[i];
        bool ordered = static_cast<int>(prev.difficulty) < static_cast<int>(cur.difficulty) ||
            (prev.difficulty == cur.difficulty && prev.title < cur.title) ||
            (prev.difficulty == cur.difficulty && prev.title == cur.title && prev.id <= cur.id);
        if (!ordered) {
            throw std::runtime_error("Quests out of order: " + prev.id + " before " + cur.id);
        }
    }
END_TEST()

TEST("Guided path is first_steps, then reading_logs, then crashloop")
    QuestManager quests("src/modules");
    quests.load_all_quests();

    const Quest* first = quests.get_quest("kubernetes", "k8s_first_steps");
    const Quest* logs = quests.get_quest("kubernetes", "k8s_reading_logs");
    const Quest* crash = quests.get_quest("kubernetes", "k8s_crashloop");
    ASSERT_TRUE(first != nullptr && logs != nullptr && crash != nullptr);
    ASSERT_TRUE(std::find(first->unlock_quests.begin(), first->unlock_quests.end(), "k8s_reading_logs") != first->unlock_quests.end());
    ASSERT_TRUE(std::find(logs->unlock_quests.begin(), logs->unlock_quests.end(), "k8s_crashloop") != logs->unlock_quests.end());

    std::string next_logs = quests.format_next_step("kubernetes", "k8s_first_steps");
    ASSERT_TRUE(next_logs.rfind("Next:", 0) == 0);
    ASSERT_TRUE(next_logs.find("k8s_reading_logs") != std::string::npos);

    std::string next_crash = quests.format_next_step("kubernetes", "k8s_reading_logs");
    ASSERT_TRUE(next_crash.rfind("Next:", 0) == 0);
    ASSERT_TRUE(next_crash.find("k8s_crashloop") != std::string::npos);
END_TEST()

TEST("Unfinished quests are hidden, and playable hints only mention supported kubectl")
    QuestManager quests("src/modules");
    quests.load_all_quests();

    auto kube = std::make_shared<KubernetesModule>();
    ASSERT_TRUE(kube->load_from_path("src/modules/kubernetes"));
    auto prefixes = kube->registered_prefixes();

    std::set<std::string> hidden(std::begin(kHiddenQuests), std::end(kHiddenQuests));
    auto loaded = quests.get_quests_for_module("kubernetes");

    for (const auto& quest : loaded) {
        bool is_hidden = hidden.count(quest.id) > 0;
        if (quest.available == is_hidden) {
            throw std::runtime_error(std::string("available flag mismatch for ") + quest.id);
        }

        bool saw_unsupported = false;
        for (const auto& hint : quest.hints) {
            for (const auto& cmd : extract_kubectl_commands(hint)) {
                if (!kubectl_invocation_supported(prefixes, cmd)) {
                    saw_unsupported = true;
                    if (quest.available) {
                        throw std::runtime_error(quest.id + " hint uses unsupported command: " + cmd);
                    }
                }
            }
        }
        if (is_hidden && !saw_unsupported) {
            throw std::runtime_error(quest.id + " is hidden but every hint command is implemented; unhide it");
        }
    }

    quests.mark_quest_completed("kubernetes", "k8s_rolling_update");
    ASSERT_TRUE(!quests.activate_quest("kubernetes", "k8s_node_drain"));
    std::string listing = quests.list_quests("kubernetes");
    std::string grouped = quests.list_all_quests_grouped();
    for (const char* id : kHiddenQuests) {
        if (listing.find(id) != std::string::npos || grouped.find(id) != std::string::npos) {
            throw std::runtime_error(std::string("Hidden quest is still listed: ") + id);
        }
    }
END_TEST()

TEST("Unsupported kubectl is an honest simulator error, and common aliases work")
    auto kube = std::make_shared<KubernetesModule>();
    ASSERT_TRUE(kube->load_from_path("src/modules/kubernetes"));
    Router router;
    router.add_module(kube);

    std::string missing = router.handle_command({"kubectl", "get", "ingress"});
    ASSERT_TRUE(missing.find("command not found") == std::string::npos);
    ASSERT_TRUE(missing.find("-bash:") == std::string::npos);
    ASSERT_TRUE(missing.find("not implemented") != std::string::npos);
    ASSERT_TRUE(missing.find("kubectl get ingress") != std::string::npos);

    std::string unknown = router.handle_command({"not-a-real-binary"});
    ASSERT_TRUE(unknown.find("command not found") != std::string::npos);

    std::string pods = router.handle_command({"kubectl", "get", "pods"});
    std::string po = router.handle_command({"kubectl", "get", "po"});
    std::string pod = router.handle_command({"kubectl", "get", "pod"});
    ASSERT_EQ(po, pods);
    ASSERT_EQ(pod, pods);
    ASSERT_TRUE(pods.find("backend") != std::string::npos);

    std::string nodes = router.handle_command({"kubectl", "get", "nodes"});
    std::string no = router.handle_command({"kubectl", "get", "no"});
    ASSERT_EQ(no, nodes);
    ASSERT_TRUE(nodes.find("node-1") != std::string::npos);

    std::string described_pod = router.handle_command({"kubectl", "describe", "pod", "backend"});
    std::string described_po = router.handle_command({"kubectl", "describe", "po", "backend"});
    ASSERT_EQ(described_po, described_pod);

    std::string deployments = router.handle_command({"kubectl", "get", "deployments"});
    std::string deployment = router.handle_command({"kubectl", "get", "deployment"});
    std::string deploy = router.handle_command({"kubectl", "get", "deploy"});
    ASSERT_EQ(deployment, deployments);
    ASSERT_EQ(deploy, deployments);

    std::string describe_deploy = router.handle_command({"kubectl", "describe", "deploy", "missing"});
    std::string describe_deployment = router.handle_command({"kubectl", "describe", "deployment", "missing"});
    ASSERT_EQ(describe_deploy, describe_deployment);
    ASSERT_TRUE(describe_deploy.find("not implemented") == std::string::npos);
    ASSERT_TRUE(describe_deploy.find("command not found") == std::string::npos);
    ASSERT_TRUE(describe_deploy.find("not found") != std::string::npos);

    auto prefixes = kube->registered_prefixes();
    ASSERT_TRUE(std::find(prefixes.begin(), prefixes.end(), "kubectl edit deploy") != prefixes.end());
    ASSERT_TRUE(std::find(prefixes.begin(), prefixes.end(), "kubectl describe no") != prefixes.end());
    ASSERT_TRUE(std::find(prefixes.begin(), prefixes.end(), "kubectl delete po") != prefixes.end());
END_TEST()

TEST("First launch starts k8s_first_steps and completion prints Next")
    fs::path fresh_home = home / "new-player";
    fs::create_directories(fresh_home);
    setenv("HOME", fresh_home.c_str(), 1);

    Engine engine("src/modules");
    ASSERT_TRUE(engine.get_settings().is_telemetry_enabled() == false);
    ASSERT_TRUE(engine.get_settings().has_telemetry_preference() == false);
    ASSERT_TRUE(engine.get_telemetry().is_telemetry_enabled() == false);

    auto kube = std::make_shared<KubernetesModule>();
    ASSERT_TRUE(kube->load_from_path("src/modules/kubernetes"));
    engine.add_module(kube);

    std::string banner = engine.begin_first_quest_if_needed();
    ASSERT_TRUE(banner.find("QUEST ACTIVATED") != std::string::npos);
    ASSERT_TRUE(banner.find("First Steps") != std::string::npos);
    ASSERT_TRUE(engine.get_quest_manager().is_active("kubernetes", "k8s_first_steps"));
    ASSERT_TRUE(engine.begin_first_quest_if_needed().empty());

    std::string done = engine.dispatch_command("kubectl version");
    ASSERT_TRUE(done.find("QUEST COMPLETE") != std::string::npos);
    ASSERT_TRUE(done.find("Next:") != std::string::npos);
    ASSERT_TRUE(done.find("k8s_reading_logs") != std::string::npos);

    Engine returning("src/modules");
    ASSERT_TRUE(returning.begin_first_quest_if_needed().empty());
END_TEST()

    fs::remove_all(home);

    std::cout << "\n========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << tests_passed << "\n";
    std::cout << "Tests Failed: " << tests_failed << "\n";

    if (tests_failed == 0) {
        std::cout << "\n🎉 ALL TESTS PASSED! 🎉\n\n";
        return 0;
    }
    std::cout << "\n❌ SOME TESTS FAILED ❌\n\n";
    return 1;
}
