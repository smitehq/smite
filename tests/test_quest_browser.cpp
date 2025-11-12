#include "../src/state/quest_browser.h"
#include "../src/state/quest.h"
#include "../src/core/router.h"
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
        throw std::runtime_error(std::string("Expected: ") + std::to_string(static_cast<int>(expected)) + ", Got: " + std::to_string(static_cast<int>(actual))); \
    }

#define ASSERT_STR_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        throw std::runtime_error(std::string("Expected: '") + expected + "', Got: '" + actual + "'"); \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error("Expected condition to be true"); \
    }

#define ASSERT_FALSE(condition) \
    if (condition) { \
        throw std::runtime_error("Expected condition to be false"); \
    }

#define ASSERT_GT(actual, min_value) \
    if ((actual) <= (min_value)) { \
        throw std::runtime_error(std::string("Expected value > ") + std::to_string(min_value) + ", Got: " + std::to_string(actual)); \
    }

#define ASSERT_DIFFICULTY_EQ(actual, expected) \
    if ((actual) != (expected)) { \
        auto diff_to_str = [](QuestDifficulty d) -> std::string { \
            switch (d) { \
                case QuestDifficulty::BEGINNER: return "BEGINNER"; \
                case QuestDifficulty::INTERMEDIATE: return "INTERMEDIATE"; \
                case QuestDifficulty::ADVANCED: return "ADVANCED"; \
                case QuestDifficulty::EXPERT: return "EXPERT"; \
                default: return "UNKNOWN"; \
            } \
        }; \
        throw std::runtime_error(std::string("Expected difficulty: ") + diff_to_str(expected) + ", Got: " + diff_to_str(actual)); \
    }

int main() {
    std::cout << "========================================\n";
    std::cout << "Quest Browser Automated Test Suite\n";
    std::cout << "========================================\n";

    // Set up test environment
    QuestManager quest_mgr("src/modules");
    Router router;
    auto k8s_module = std::make_shared<KubernetesModule>();

    TEST("QuestManager loads all quests from modules")
        quest_mgr.load_all_quests();
        auto k8s_quests = quest_mgr.get_quests_for_module("kubernetes");
        std::cout << "Loaded " << k8s_quests.size() << " kubernetes quests\n";
        ASSERT_GT(k8s_quests.size(), 0);
    END_TEST()

    TEST("Router can add kubernetes module")
        bool loaded = k8s_module->load_from_path("src/modules/kubernetes");
        ASSERT_TRUE(loaded);
        router.add_module(k8s_module);
        auto modules = router.get_modules();
        ASSERT_EQ(modules.size(), 1);
    END_TEST()

    TEST("QuestBrowser can be constructed")
        QuestBrowser browser(quest_mgr, router);
        std::cout << "QuestBrowser successfully constructed\n";
    END_TEST()

    TEST("QuestManager can retrieve specific quest")
        const Quest* quest = quest_mgr.get_quest("kubernetes", "k8s_first_steps");
        ASSERT_TRUE(quest != nullptr);
        ASSERT_STR_EQ(quest->id, "k8s_first_steps");
        ASSERT_STR_EQ(quest->title, "First Steps in Kubernetes");
        ASSERT_DIFFICULTY_EQ(quest->difficulty, QuestDifficulty::BEGINNER);
        ASSERT_EQ(quest->reward_xp, 25);
        std::cout << "Quest: " << quest->title << " (" << quest->reward_xp << " XP)\n";
    END_TEST()

    TEST("QuestManager can retrieve quests by module")
        auto quests = quest_mgr.get_quests_for_module("kubernetes");
        ASSERT_GT(quests.size(), 0);

        // Verify all quests have required fields
        for (const auto& q : quests) {
            ASSERT_TRUE(!q.id.empty());
            ASSERT_TRUE(!q.title.empty());
            ASSERT_TRUE(!q.description.empty());
            ASSERT_GT(q.reward_xp, 0);
        }

        std::cout << "Verified " << quests.size() << " quests have valid data\n";
    END_TEST()

    TEST("QuestManager can activate and track quests")
        bool activated = quest_mgr.activate_quest("kubernetes", "k8s_first_steps");
        ASSERT_TRUE(activated);

        bool is_active = quest_mgr.is_active("kubernetes", "k8s_first_steps");
        ASSERT_TRUE(is_active);

        bool other_not_active = quest_mgr.is_active("kubernetes", "k8s_crashloop");
        ASSERT_FALSE(other_not_active);

        std::cout << "Quest activation and tracking works correctly\n";
    END_TEST()

    TEST("QuestManager can provide hints for active quest")
        // First hint
        std::string hint1 = quest_mgr.get_next_hint("kubernetes");
        ASSERT_TRUE(!hint1.empty());
        ASSERT_TRUE(hint1.find("Hint 1") != std::string::npos);
        std::cout << "First hint retrieved successfully\n";

        // Second hint
        std::string hint2 = quest_mgr.get_next_hint("kubernetes");
        ASSERT_TRUE(!hint2.empty());
        ASSERT_TRUE(hint2.find("Hint 2") != std::string::npos);
        std::cout << "Second hint retrieved successfully\n";

        // Check hints shown count
        int hints_shown = quest_mgr.get_hints_shown("kubernetes");
        ASSERT_EQ(hints_shown, 2);
    END_TEST()

    TEST("QuestManager handles exhausted hints gracefully")
        // k8s_first_steps has 2 hints, we've shown both, so next should say no more hints
        std::string hint3 = quest_mgr.get_next_hint("kubernetes");
        ASSERT_TRUE(!hint3.empty());
        ASSERT_TRUE(hint3.find("No More Hints") != std::string::npos);
        std::cout << "Exhausted hints message shown correctly\n";
    END_TEST()

    TEST("QuestManager can list all quests grouped by module and difficulty")
        std::string listing = quest_mgr.list_all_quests_grouped();
        ASSERT_TRUE(!listing.empty());
        ASSERT_TRUE(listing.find("QUEST BROWSER") != std::string::npos);
        ASSERT_TRUE(listing.find("kubernetes") != std::string::npos);
        ASSERT_TRUE(listing.find("Beginner") != std::string::npos);
        ASSERT_TRUE(listing.find("k8s_first_steps") != std::string::npos);
        std::cout << "Quest listing generated successfully\n";
    END_TEST()

    TEST("QuestManager can activate a different quest (only one active per module)")
        bool activated = quest_mgr.activate_quest("kubernetes", "k8s_crashloop");
        ASSERT_TRUE(activated);

        bool new_active = quest_mgr.is_active("kubernetes", "k8s_crashloop");
        ASSERT_TRUE(new_active);

        bool old_not_active = quest_mgr.is_active("kubernetes", "k8s_first_steps");
        ASSERT_FALSE(old_not_active);

        // Hints should be reset for new quest
        int hints_shown = quest_mgr.get_hints_shown("kubernetes");
        ASSERT_EQ(hints_shown, 0);

        std::cout << "Quest switching works correctly\n";
    END_TEST()

    TEST("QuestManager list_quests shows active quest marker")
        std::string listing = quest_mgr.list_quests("kubernetes");
        ASSERT_TRUE(!listing.empty());
        ASSERT_TRUE(listing.find("k8s_crashloop") != std::string::npos);
        ASSERT_TRUE(listing.find("[ACTIVE]") != std::string::npos);
        std::cout << "Quest listing shows active marker\n";
    END_TEST()

    TEST("Quest completion message is formatted correctly")
        std::string completion = QuestManager::quest_accomplished(
            "You have defeated the bug!\n\nREWARDS:\n * +50 XP\n * Achievement: Bug Slayer"
        );
        ASSERT_TRUE(!completion.empty());
        ASSERT_TRUE(completion.find("QUEST COMPLETE!") != std::string::npos);
        ASSERT_TRUE(completion.find("Bug Slayer") != std::string::npos);
        std::cout << "Quest completion message formatted correctly\n";
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
