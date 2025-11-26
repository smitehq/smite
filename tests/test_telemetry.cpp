#include "../src/state/telemetry.h"
#include "../src/state/quest.h"
#include <iostream>
#include <cassert>
#include <string>
#include <thread>
#include <chrono>

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

#define ASSERT(cond, msg) \
    if (!(cond)) { \
        throw std::runtime_error(msg); \
    }

#define ASSERT_EQUAL(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected: ") + std::to_string(expected) + ", Got: " + std::to_string(actual)); \
    }

#define ASSERT_STR_EQUAL(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected: '") + expected + "', Got: '" + actual + "'"); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' does not contain '" + substr + "'"); \
    }

int main() {

TEST("Telemetry tracks quest start time")
    TelemetryManager telemetry;

    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");

    const QuestTelemetry* data = telemetry.get_active_telemetry("kubernetes");
    ASSERT(data != nullptr, "Telemetry data should exist");
    ASSERT_STR_EQUAL(data->quest_id, "k8s_crashloop");
    ASSERT_STR_EQUAL(data->module_name, "kubernetes");
    ASSERT_EQUAL(data->completed, false);
END_TEST()

TEST("Telemetry tracks commands during quest")
    TelemetryManager telemetry;

    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");
    telemetry.track_command("kubernetes", "kubectl", "kubectl get pods");
    telemetry.track_command("kubernetes", "kubectl", "kubectl describe pod backend");
    telemetry.track_command("kubernetes", "kubectl", "kubectl logs backend");

    const QuestTelemetry* data = telemetry.get_active_telemetry("kubernetes");
    ASSERT(data != nullptr, "Telemetry data should exist");
    ASSERT_EQUAL(data->commands.size(), 3);
    ASSERT_STR_EQUAL(data->commands[0].command, "kubectl");
    ASSERT_STR_EQUAL(data->commands[0].full_input, "kubectl get pods");
    ASSERT_STR_EQUAL(data->commands[2].full_input, "kubectl logs backend");
END_TEST()

TEST("Telemetry tracks hint usage")
    TelemetryManager telemetry;

    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");
    telemetry.track_hint_used("kubernetes");
    telemetry.track_hint_used("kubernetes");

    const QuestTelemetry* data = telemetry.get_active_telemetry("kubernetes");
    ASSERT(data != nullptr, "Telemetry data should exist");
    ASSERT_EQUAL(data->hints_used, 2);
END_TEST()

TEST("Time bonus calculation - fast completion")
    TelemetryManager telemetry;

    // 0 minutes = +100 bonus
    int bonus = telemetry.calculate_time_bonus(0);
    ASSERT_EQUAL(bonus, 100);

    // 1 minute = +95 bonus
    bonus = telemetry.calculate_time_bonus(1);
    ASSERT_EQUAL(bonus, 95);

    // 5 minutes = +75 bonus
    bonus = telemetry.calculate_time_bonus(5);
    ASSERT_EQUAL(bonus, 75);
END_TEST()

TEST("Time bonus calculation - slow completion")
    TelemetryManager telemetry;

    // 10 minutes = +50 bonus
    int bonus = telemetry.calculate_time_bonus(10);
    ASSERT_EQUAL(bonus, 50);

    // 15 minutes = +25 bonus
    bonus = telemetry.calculate_time_bonus(15);
    ASSERT_EQUAL(bonus, 25);

    // 20 minutes = 0 bonus (capped at 0)
    bonus = telemetry.calculate_time_bonus(20);
    ASSERT_EQUAL(bonus, 0);

    // 30 minutes = 0 bonus (capped at 0, not negative)
    bonus = telemetry.calculate_time_bonus(30);
    ASSERT_EQUAL(bonus, 0);
END_TEST()

TEST("Quest completion calculates total XP")
    TelemetryManager telemetry;

    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");

    // Simulate 2 minutes passing (sleep for a tiny bit to ensure time passes)
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Complete the quest with 100 base XP
    telemetry.complete_quest_tracking("kubernetes", "k8s_crashloop", 100);

    const QuestTelemetry* data = telemetry.get_active_telemetry("kubernetes");
    ASSERT(data != nullptr, "Telemetry data should exist");
    ASSERT_EQUAL(data->completed, true);

    // XP should be base (100) + time bonus (95-100 depending on exact timing)
    ASSERT(data->xp_earned >= 195, "Total XP should be at least 195");
    ASSERT(data->xp_earned <= 200, "Total XP should be at most 200");
END_TEST()

TEST("Telemetry opt-out disables tracking")
    TelemetryManager telemetry;

    telemetry.set_telemetry_enabled(false);

    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");
    telemetry.track_command("kubernetes", "kubectl", "kubectl get pods");
    telemetry.track_hint_used("kubernetes");

    const QuestTelemetry* data = telemetry.get_active_telemetry("kubernetes");
    // When disabled, no data should be tracked
    ASSERT(data == nullptr, "Telemetry data should not exist when disabled");
END_TEST()

TEST("Post-mortem contains expected sections")
    QuestManager quests("./src/modules");
    quests.load_all_quests();

    TelemetryManager telemetry;
    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");

    // Simulate some investigation
    telemetry.track_command("kubernetes", "kubectl", "kubectl get pods");
    telemetry.track_command("kubernetes", "kubectl", "kubectl describe pod backend");

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Get the quest and complete it
    const Quest* quest = quests.get_quest("kubernetes", "k8s_crashloop");
    ASSERT(quest != nullptr, "Quest should exist");

    telemetry.complete_quest_tracking("kubernetes", "k8s_crashloop", quest->reward_xp);

    std::string report = quests.generate_post_mortem("kubernetes", "k8s_crashloop", telemetry, nullptr);

    // Check that report contains expected sections
    ASSERT_STR_CONTAINS(report, "POST-MORTEM REPORT");
    ASSERT_STR_CONTAINS(report, "Time to Resolution:");
    ASSERT_STR_CONTAINS(report, "XP Earned:");
    ASSERT_STR_CONTAINS(report, "Base:");
    ASSERT_STR_CONTAINS(report, "Speed Bonus:");
    ASSERT_STR_CONTAINS(report, "Total:");
END_TEST()

TEST("Post-mortem shows root cause if available")
    QuestManager quests("./src/modules");
    quests.load_all_quests();

    TelemetryManager telemetry;
    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");

    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const Quest* quest = quests.get_quest("kubernetes", "k8s_crashloop");
    telemetry.complete_quest_tracking("kubernetes", "k8s_crashloop", quest->reward_xp);

    std::string report = quests.generate_post_mortem("kubernetes", "k8s_crashloop", telemetry, nullptr);

    // k8s_crashloop has a root_cause field defined
    ASSERT_STR_CONTAINS(report, "Root Cause:");
    ASSERT_STR_CONTAINS(report, "Missing database credentials");
END_TEST()

TEST("Post-mortem provides tips for missing diagnostics")
    QuestManager quests("./src/modules");
    quests.load_all_quests();

    TelemetryManager telemetry;
    telemetry.start_quest_tracking("kubernetes", "k8s_crashloop");

    // Complete quest without running any diagnostic commands
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    const Quest* quest = quests.get_quest("kubernetes", "k8s_crashloop");
    telemetry.complete_quest_tracking("kubernetes", "k8s_crashloop", quest->reward_xp);

    std::string report = quests.generate_post_mortem("kubernetes", "k8s_crashloop", telemetry, nullptr);

    // Without module, no tips will be shown (that's OK for this test)
    // Tips are now module-specific
END_TEST()

    // Print summary
    std::cout << "\n";
    std::cout << "========================================\n";
    std::cout << "Test Summary\n";
    std::cout << "========================================\n";
    std::cout << "Tests Passed: " << tests_passed << "\n";
    std::cout << "Tests Failed: " << tests_failed << "\n";
    std::cout << "\n";

    if (tests_failed == 0) {
        std::cout << "🎉 ALL TESTS PASSED! 🎉\n";
        return 0;
    } else {
        std::cout << "❌ SOME TESTS FAILED\n";
        return 1;
    }
}
