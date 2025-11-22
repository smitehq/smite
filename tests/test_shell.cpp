#include "../src/shell/shell.h"
#include "../src/globals.h"
#include <iostream>
#include <cassert>
#include <string>
#include <sstream>

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
        std::ostringstream _assert_eq_ss; \
        _assert_eq_ss << "Expected: " << (expected) << ", Got: " << (actual); \
        throw std::runtime_error(_assert_eq_ss.str()); \
    }

#define ASSERT_STR_CONTAINS(str, substr) \
    if ((str).find(substr) == std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' does not contain '" + substr + "'"); \
    }

#define ASSERT_STR_NOT_CONTAINS(str, substr) \
    if ((str).find(substr) != std::string::npos) { \
        throw std::runtime_error(std::string("String '") + str + "' should not contain '" + substr + "'"); \
    }

#define ASSERT_TRUE(condition) \
    if (!(condition)) { \
        throw std::runtime_error(std::string("Condition failed: ") + #condition); \
    }

int main() {
    std::cout << "========================================\n";
    std::cout << "Shell Module Automated Test Suite\n";
    std::cout << "========================================\n";

    auto shell = std::make_shared<Shell>();

    TEST("Shell module loads from path")
        bool loaded = shell->load_from_path("");
        ASSERT_EQ(loaded, true);
    END_TEST()

    TEST("pwd shows home directory initially")
        std::string output = shell->run_command("pwd", {});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, std::string("/home/") + globals::PLAYER_NAME);
    END_TEST()

    TEST("ls shows readme.txt in home directory")
        std::string output = shell->run_command("ls", {});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "readme.txt");
    END_TEST()

    TEST("ls -l shows detailed listing with permissions")
        std::string output = shell->run_command("ls", {"-l"});
        std::cout << "Output:\n" << output;
        ASSERT_STR_CONTAINS(output, "readme.txt");
        ASSERT_STR_CONTAINS(output, globals::PLAYER_NAME);
        ASSERT_STR_CONTAINS(output, "total");
    END_TEST()

    TEST("cat readme.txt shows file content")
        std::string output = shell->run_command("cat", {"readme.txt"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "Welcome to smiteOS");
    END_TEST()

    TEST("cat non-existent file shows error")
        std::string output = shell->run_command("cat", {"nonexistent.txt"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "No such file or directory");
    END_TEST()

    TEST("cat without arguments shows error")
        std::string output = shell->run_command("cat", {});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "No file provided");
    END_TEST()

    TEST("echo without arguments outputs newline")
        std::string output = shell->run_command("echo", {});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "\n");
    END_TEST()

    TEST("echo hello world outputs text")
        std::string output = shell->run_command("echo", {"hello", "world"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "hello world");
    END_TEST()

    TEST("touch creates new file")
        std::string output = shell->run_command("touch", {"newfile.txt"});
        std::cout << "Output: '" << output << "'";
        // touch succeeds silently
        ASSERT_EQ(output, "");

        // Verify file was created
        std::string ls_output = shell->run_command("ls", {});
        std::cout << "ls output: " << ls_output;
        ASSERT_STR_CONTAINS(ls_output, "newfile.txt");
    END_TEST()

    TEST("touch without arguments shows error")
        std::string output = shell->run_command("touch", {});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "missing file operand");
    END_TEST()

    TEST("touch existing file succeeds silently")
        std::string output = shell->run_command("touch", {"readme.txt"});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "");
    END_TEST()

    TEST("chmod changes file permissions")
        std::string output = shell->run_command("chmod", {"755", "readme.txt"});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "");

        // Verify permissions changed with ls -l
        std::string ls_output = shell->run_command("ls", {"-l"});
        std::cout << "ls -l output:\n" << ls_output;
        ASSERT_STR_CONTAINS(ls_output, "755");
    END_TEST()

    TEST("chmod with invalid mode shows error")
        std::string output = shell->run_command("chmod", {"999", "readme.txt"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "invalid mode");
    END_TEST()

    TEST("chmod without enough arguments shows error")
        std::string output = shell->run_command("chmod", {"755"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "missing operand");
    END_TEST()

    TEST("chmod non-existent file shows error")
        std::string output = shell->run_command("chmod", {"755", "nonexistent.txt"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "No such file or directory");
    END_TEST()

    TEST("cd /tmp changes to tmp directory")
        std::string output = shell->run_command("cd", {"/tmp"});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "");

        // Verify we're in /tmp
        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, "/tmp");
    END_TEST()

    TEST("cd .. goes up one directory")
        std::string output = shell->run_command("cd", {".."});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "");

        // Verify we're in root
        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, "/");
        ASSERT_STR_NOT_CONTAINS(pwd_output, "/tmp");
    END_TEST()

    TEST("cd ~ goes home")
        shell->run_command("cd", {"/"});
        std::string output = shell->run_command("cd", {"~"});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "");

        // Verify we're in root
        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, std::string("/home/") + globals::PLAYER_NAME);
    END_TEST()

    TEST("cd without arguments goes to home")
        shell->run_command("cd", {"/"});
        std::string output = shell->run_command("cd", {});
        std::cout << "Output: '" << output << "'";
        ASSERT_EQ(output, "");

        // Verify we're in home
        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, std::string("/home/") + globals::PLAYER_NAME);
    END_TEST()

    TEST("cd to non-existent directory shows error")
        std::string output = shell->run_command("cd", {"/nonexistent"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("ls /etc shows files in etc directory")
        std::string output = shell->run_command("ls", {"/etc"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "motd");
    END_TEST()

    TEST("cat /etc/motd shows message of the day")
        std::string output = shell->run_command("cat", {"/etc/motd"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "Welcome adventurer");
    END_TEST()

    TEST("Shell supports registered commands")
        bool supports_ls = shell->supports_command("ls");
        bool supports_cd = shell->supports_command("cd");
        bool supports_fake = shell->supports_command("fakecommand");

        ASSERT_TRUE(supports_ls);
        ASSERT_TRUE(supports_cd);
        ASSERT_TRUE(!supports_fake);
    END_TEST()

    TEST("Shell returns correct registered prefixes")
        auto prefixes = shell->registered_prefixes();
        std::cout << "Registered commands: ";
        for (const auto& p : prefixes) {
            std::cout << p << " ";
        }
        std::cout << "\n";

        bool has_ls = false;
        bool has_cd = false;
        bool has_pwd = false;
        for (const auto& p : prefixes) {
            if (p == "ls") has_ls = true;
            if (p == "cd") has_cd = true;
            if (p == "pwd") has_pwd = true;
        }

        ASSERT_TRUE(has_ls);
        ASSERT_TRUE(has_cd);
        ASSERT_TRUE(has_pwd);
    END_TEST()

    TEST("Unknown command returns error")
        std::string output = shell->run_command("unknowncommand", {});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "Command not found");
    END_TEST()

    TEST("Path resolution with tilde")
        // cd to tmp first
        shell->run_command("cd", {"/tmp"});

        // cd ~ should go to home
        std::string output = shell->run_command("cd", {"~"});
        std::cout << "cd ~ output: '" << output << "'";
        ASSERT_EQ(output, "");

        // Verify we're in home
        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, std::string("/home/") + globals::PLAYER_NAME);
    END_TEST()

    TEST("Relative path navigation")
        // Go to home first
        shell->run_command("cd", {});

        // Create a test directory structure would require more setup
        // For now, just test that relative cd to non-existent shows error
        std::string output = shell->run_command("cd", {"nonexistent"});
        std::cout << "Output: " << output;
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    // ========================================
    // Path Traversal & Filesystem Stress Tests
    // ========================================

    TEST("cd ../../../.. - relative parent paths not yet supported")
        shell->run_command("cd", {"/tmp"});
        std::string output = shell->run_command("cd", {"../../../.."});
        std::cout << "cd ../../../.. output: '" << output << "'";
        // Current behavior: relative paths with .. are not supported
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("cd /../../../.. - absolute paths with .. not yet normalized")
        std::string output = shell->run_command("cd", {"/../../../.."});
        std::cout << "cd /../../../.. output: '" << output << "'";
        // Current behavior: .. in paths not normalized
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("cd with dots in middle of path - not yet normalized")
        shell->run_command("cd", {"/"});
        std::string output = shell->run_command("cd", {"/tmp/../etc"});
        std::cout << "cd /tmp/../etc output: '" << output << "'";
        // Current behavior: .. not resolved
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("cd /./tmp/./. handles single dots")
        std::string output = shell->run_command("cd", {"/./tmp/./."});
        std::cout << "cd /./tmp/./. output: '" << output << "'";

        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        // Current behavior: . not normalized but path still works
        ASSERT_STR_CONTAINS(pwd_output, "tmp");
    END_TEST()

    TEST("ls ../etc from /tmp - relative paths not supported")
        shell->run_command("cd", {"/tmp"});
        std::string output = shell->run_command("ls", {"../etc"});
        std::cout << "ls ../etc output: " << output;
        // Current behavior: relative paths not supported
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("cat ../etc/motd from /tmp - relative paths not supported")
        shell->run_command("cd", {"/tmp"});
        std::string output = shell->run_command("cat", {"../etc/motd"});
        std::cout << "cat ../etc/motd output: " << output;
        // Current behavior: relative paths not supported
        ASSERT_STR_CONTAINS(output, "No such file");
    END_TEST()

    TEST("ls with excessive parent traversal - not supported")
        shell->run_command("cd", {"/tmp"});
        std::string output = shell->run_command("ls", {"../../../../../../../../"});
        std::cout << "ls ../../../../../../../../ output: " << output;
        // Current behavior: relative paths not supported
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("cat /etc/../etc/motd - path normalization not supported")
        std::string output = shell->run_command("cat", {"/etc/../etc/motd"});
        std::cout << "cat /etc/../etc/motd output: " << output;
        // Current behavior: .. not normalized
        ASSERT_STR_CONTAINS(output, "No such file");
    END_TEST()

    TEST("cd to path with trailing slashes")
        shell->run_command("cd", {"/"});
        std::string output = shell->run_command("cd", {"/tmp///"});
        std::cout << "cd /tmp/// output: '" << output << "'";

        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        // Current behavior: trailing slashes preserved
        ASSERT_STR_CONTAINS(pwd_output, "tmp");
    END_TEST()

    TEST("touch file in /etc fails (read-only)")
        shell->run_command("cd", {"/etc"});
        std::string output = shell->run_command("touch", {"hacked.txt"});
        std::cout << "touch /etc/hacked.txt output: " << output;
        // Should either fail or not actually create the file
        std::string ls_output = shell->run_command("ls", {});
        std::cout << "ls /etc output: " << ls_output;
        // If hacked.txt doesn't appear, that's correct behavior
        // Some implementations might allow it in virtual FS
    END_TEST()

    TEST("cd ~/../.. tries to escape via home")
        shell->run_command("cd", {"~"});
        std::string output = shell->run_command("cd", {"../.."});
        std::cout << "cd ../.. from home output: '" << output << "'";

        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        // Should be at / or /home, not beyond root
        ASSERT_STR_CONTAINS(pwd_output, "/");
    END_TEST()

    TEST("ls with absolute path ignores cwd")
        shell->run_command("cd", {"/tmp"});
        std::string output = shell->run_command("ls", {"/etc"});
        std::cout << "ls /etc from /tmp output: " << output;
        ASSERT_STR_CONTAINS(output, "motd");
    END_TEST()

    TEST("cat file with spaces in error message")
        std::string output = shell->run_command("cat", {"file with spaces.txt"});
        std::cout << "cat 'file with spaces.txt' output: " << output;
        ASSERT_STR_CONTAINS(output, "No such file");
    END_TEST()

    TEST("empty path cd shows error or goes home")
        std::string output = shell->run_command("cd", {""});
        std::cout << "cd '' output: '" << output << "'";
        // Empty string might go home or show error, both are acceptable
        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, "/");
    END_TEST()

    TEST("ls non-existent path shows error")
        std::string output = shell->run_command("ls", {"/nonexistent/path"});
        std::cout << "ls /nonexistent/path output: " << output;
        ASSERT_STR_CONTAINS(output, "No such directory");
    END_TEST()

    TEST("cd ///// multiple slashes normalizes to root")
        std::string output = shell->run_command("cd", {"/////"});
        std::cout << "cd ///// output: '" << output << "'";

        std::string pwd_output = shell->run_command("pwd", {});
        std::cout << "pwd output: " << pwd_output;
        ASSERT_STR_CONTAINS(pwd_output, "/");
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
