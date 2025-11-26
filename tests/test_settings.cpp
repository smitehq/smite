#include "../src/state/settings.h"
#include <iostream>
#include <cassert>
#include <filesystem>

namespace fs = std::filesystem;

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

int main() {

TEST("Settings file path contains .smite directory")
    fs::path settings_path = SettingsManager::get_settings_path();
    std::string path_str = settings_path.string();

    ASSERT(path_str.find(".smite") != std::string::npos,
           "Settings path should contain .smite directory");
    ASSERT(path_str.find("settings.yaml") != std::string::npos,
           "Settings path should end with settings.yaml");
END_TEST()

TEST("New SettingsManager defaults to enabled with no preference")
    // Delete settings file if it exists
    fs::path settings_path = SettingsManager::get_settings_path();
    if (fs::exists(settings_path)) {
        fs::remove(settings_path);
    }

    SettingsManager settings;
    ASSERT(settings.is_telemetry_enabled() == true,
           "Default telemetry should be enabled");
    ASSERT(settings.has_telemetry_preference() == false,
           "Should not have preference on first run");
END_TEST()

TEST("Setting telemetry preference saves to disk")
    // Delete settings file if it exists
    fs::path settings_path = SettingsManager::get_settings_path();
    if (fs::exists(settings_path)) {
        fs::remove(settings_path);
    }

    {
        SettingsManager settings;
        settings.set_telemetry_enabled(false);
        ASSERT(settings.has_telemetry_preference() == true,
               "Should have preference after setting");
    }

    // Load in a new instance to verify persistence
    {
        SettingsManager settings;
        ASSERT(settings.is_telemetry_enabled() == false,
               "Telemetry should be disabled after loading from disk");
        ASSERT(settings.has_telemetry_preference() == true,
               "Should have preference after loading from disk");
    }
END_TEST()

TEST("Settings can be toggled between enabled and disabled")
    fs::path settings_path = SettingsManager::get_settings_path();
    if (fs::exists(settings_path)) {
        fs::remove(settings_path);
    }

    SettingsManager settings;

    settings.set_telemetry_enabled(true);
    ASSERT(settings.is_telemetry_enabled() == true, "Should be enabled");

    settings.set_telemetry_enabled(false);
    ASSERT(settings.is_telemetry_enabled() == false, "Should be disabled");

    settings.set_telemetry_enabled(true);
    ASSERT(settings.is_telemetry_enabled() == true, "Should be enabled again");
END_TEST()

TEST("Settings directory is created if it doesn't exist")
    // Delete the entire .smite directory
    fs::path settings_path = SettingsManager::get_settings_path();
    fs::path settings_dir = settings_path.parent_path();

    if (fs::exists(settings_dir)) {
        fs::remove_all(settings_dir);
    }

    ASSERT(!fs::exists(settings_dir), "Settings directory should not exist");

    SettingsManager settings;
    settings.set_telemetry_enabled(true);

    ASSERT(fs::exists(settings_dir), "Settings directory should be created");
    ASSERT(fs::exists(settings_path), "Settings file should be created");
END_TEST()

    // Clean up
    fs::path settings_path = SettingsManager::get_settings_path();
    if (fs::exists(settings_path)) {
        fs::remove(settings_path);
    }

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
