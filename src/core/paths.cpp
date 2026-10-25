#include "paths.h"

#include <cstdlib>
#include <iterator>
#include <system_error>
#include <vector>

#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#include <cstdint>
#endif

namespace smite::paths {
namespace {

bool looks_like_modules_dir(const std::filesystem::path& dir) {
    std::error_code ec;
    // Object trees also contain a kubernetes/ directory (build/src/modules/kubernetes).
    // Require cluster state so we only accept the data directory.
    return std::filesystem::is_regular_file(dir / "kubernetes" / "state" / "default.yaml", ec);
}

} // namespace

std::filesystem::path executable_path() {
#if defined(_WIN32)
    wchar_t buf[32768];
    DWORD n = GetModuleFileNameW(nullptr, buf, static_cast<DWORD>(std::size(buf)));
    if (n == 0 || n >= std::size(buf)) {
        return std::filesystem::current_path() / "smite.exe";
    }
    return std::filesystem::path(buf);
#elif defined(__APPLE__)
    char buf[4096];
    uint32_t size = sizeof(buf);
    if (_NSGetExecutablePath(buf, &size) != 0) {
        return std::filesystem::current_path() / "smite";
    }
    std::error_code ec;
    auto canonical = std::filesystem::weakly_canonical(buf, ec);
    return ec ? std::filesystem::path(buf) : canonical;
#else
    std::error_code ec;
    auto canonical = std::filesystem::canonical("/proc/self/exe", ec);
    if (ec) {
        return std::filesystem::current_path() / "smite";
    }
    return canonical;
#endif
}

std::filesystem::path user_state_dir() {
    const char* home = std::getenv("HOME");
#if defined(_WIN32)
    if (home == nullptr || home[0] == '\0') {
        home = std::getenv("USERPROFILE");
    }
#endif
    std::filesystem::path base = (home != nullptr && home[0] != '\0')
        ? std::filesystem::path(home)
        : std::filesystem::current_path();
    return base / ".smite";
}

std::filesystem::path modules_data_dir() {
    if (const char* env = std::getenv("SMITE_DATA")) {
        if (env[0] != '\0') {
            return std::filesystem::path(env);
        }
    }

    std::vector<std::filesystem::path> roots;
    std::filesystem::path dir = executable_path().parent_path();
    for (int i = 0; i < 6 && !dir.empty(); ++i) {
        roots.push_back(dir);
        if (dir == dir.parent_path()) break;
        dir = dir.parent_path();
    }
    roots.push_back(std::filesystem::current_path());

    for (const auto& root : roots) {
        std::filesystem::path nested = root / "src" / "modules";
        if (looks_like_modules_dir(nested)) return nested;
        std::filesystem::path flat = root / "modules";
        if (looks_like_modules_dir(flat)) return flat;
    }

    return std::filesystem::current_path() / "src" / "modules";
}

} // namespace smite::paths
