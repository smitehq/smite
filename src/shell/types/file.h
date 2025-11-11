#pragma once
#include <string>
#include <chrono>
#include <ctime>
#include <format>

struct File {
    std::string content;
    std::string perms;
    std::string created_at;
    std::string modified_at;

    // Constructor
    File(const std::string& content_ = "", const std::string& perms_ = "rw-r--r--")
        : content(content_), perms(perms_)
    {
        created_at = current_time();
        modified_at = created_at;
    }

    // Set content and automatically update modified_at
    void set_content(const std::string& new_content) {
        content = new_content;
        modified_at = current_time();
    }

private:
    static std::string current_time() {
        auto now = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(now);
        std::tm local_tm{};
    #ifdef _WIN32
        localtime_s(&local_tm, &t);
    #else
        localtime_r(&t, &local_tm);
    #endif

        char buf[20];
        strftime(buf, sizeof(buf), "%b %d %H:%M", &local_tm);
        return std::string(buf); // or fmt::format("{}", buf) when C++20 is available
    }
};