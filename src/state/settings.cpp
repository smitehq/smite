#include "settings.h"
#include <yaml-cpp/yaml.h>
#include <fstream>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

namespace fs = std::filesystem;

SettingsManager::SettingsManager() {
    load();
}

std::filesystem::path SettingsManager::get_settings_path() {
    // Use .smite directory relative to current working directory (where executable runs)
    fs::path smite_dir = fs::current_path() / ".smite";
    return smite_dir / "settings.yaml";
}

void SettingsManager::ensure_settings_dir() {
    fs::path settings_path = get_settings_path();
    fs::path settings_dir = settings_path.parent_path();

    if (!fs::exists(settings_dir)) {
        try {
            fs::create_directories(settings_dir);
        } catch (const std::exception& e) {
            std::cerr << "Warning: Could not create settings directory: " << e.what() << std::endl;
        }
    }
}

void SettingsManager::load() {
    fs::path settings_path = get_settings_path();

    if (!fs::exists(settings_path)) {
        // No settings file exists yet - user hasn't made a choice
        has_preference_ = false;
        telemetry_enabled_ = true;  // Default to enabled
        return;
    }

    try {
        YAML::Node config = YAML::LoadFile(settings_path.string());

        if (config["telemetry"]) {
            if (config["telemetry"]["enabled"]) {
                telemetry_enabled_ = config["telemetry"]["enabled"].as<bool>();
                has_preference_ = true;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not load settings: " << e.what() << std::endl;
        has_preference_ = false;
        telemetry_enabled_ = true;
    }
}

void SettingsManager::save() {
    ensure_settings_dir();

    fs::path settings_path = get_settings_path();

    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "telemetry";
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "enabled";
    out << YAML::Value << telemetry_enabled_;
    out << YAML::EndMap;
    out << YAML::EndMap;

    try {
        std::ofstream fout(settings_path);
        fout << out.c_str();
        fout.close();
    } catch (const std::exception& e) {
        std::cerr << "Warning: Could not save settings: " << e.what() << std::endl;
    }
}

void SettingsManager::set_telemetry_enabled(bool enabled) {
    telemetry_enabled_ = enabled;
    has_preference_ = true;
    save();
}
