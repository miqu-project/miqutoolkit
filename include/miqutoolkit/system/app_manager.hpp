#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace miqu {

struct DesktopApp {
    std::string id;           // e.g. "firefox.desktop"
    std::string name;         // e.g. "Firefox"
    std::string generic_name; // e.g. "Web Browser"
    std::string comment;      // e.g. "Browse the World Wide Web"
    std::string exec_cmd;     // Cleaned executable command
    std::string icon_name;    // Raw icon name
    std::string icon_path;    // Resolved icon absolute file path (or empty if uncurated)
    std::string categories;
    std::string keywords;
    bool terminal = false;
    bool no_display = false;
};

class AppManager {
public:
    static AppManager* get();

    const std::vector<DesktopApp>& get_installed_apps(bool force_rescan = false);
    void scan_async(std::function<void(const std::vector<DesktopApp>&)> callback);
    void invalidate();

    static std::string clean_exec(const std::string& raw);
    static void launch(const DesktopApp& app);
    static void launch_command(const std::string& cmd, bool terminal = false);

private:
    AppManager();
    void rescan_internal();

    std::vector<DesktopApp> m_apps;
    bool m_scanned = false;
};

} // namespace miqu
