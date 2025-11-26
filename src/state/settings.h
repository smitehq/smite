#ifndef STATE_SETTINGS_H
#define STATE_SETTINGS_H

#include <string>
#include <filesystem>

class SettingsManager {
public:
    SettingsManager();

    // Telemetry settings
    bool is_telemetry_enabled() const { return telemetry_enabled_; }
    void set_telemetry_enabled(bool enabled);

    // Check if user has been prompted about telemetry
    bool has_telemetry_preference() const { return has_preference_; }

    // Save settings to disk
    void save();

    // Load settings from disk
    void load();

    // Get settings file path
    static std::filesystem::path get_settings_path();

private:
    bool telemetry_enabled_ = true;  // Default to enabled
    bool has_preference_ = false;    // Has user set a preference?

    void ensure_settings_dir();
};

#endif // STATE_SETTINGS_H
