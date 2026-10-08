#include "miqutoolkit/system/app_manager.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/thread_pool.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <set>
#include <algorithm>
#include <unistd.h>
#include <cstdlib>
#include <sys/wait.h>

#include <sys/stat.h>
#include <cstdint>

namespace miqu {

namespace fs = std::filesystem;

static std::string sanitize_theme_name(const std::string& theme) {
    if (theme.empty()) return "default";
    std::string safe = theme;
    for (char& c : safe) {
        if (!std::isalnum(c) && c != '-' && c != '_') c = '_';
    }
    return safe;
}

static std::string get_app_cache_path(const std::string& icon_theme) {
    const char* home = getenv("HOME");
    if (!home) return "";
    const char* xdg_cache = getenv("XDG_CACHE_HOME");
    std::string base = (xdg_cache && *xdg_cache) ? xdg_cache : (std::string(home) + "/.cache");
    std::string dir = base + "/miqutoolkit";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir + "/apps_" + sanitize_theme_name(icon_theme) + ".cache";
}

static int64_t get_dir_mtime(const std::string& path) {
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return static_cast<int64_t>(st.st_mtime);
    }
    return 0;
}

static void write_str(std::ofstream& out, const std::string& s) {
    uint16_t len = static_cast<uint16_t>(std::min<size_t>(s.size(), 65535));
    out.write(reinterpret_cast<const char*>(&len), sizeof(len));
    if (len > 0) {
        out.write(s.data(), len);
    }
}

static std::string read_str(std::ifstream& in) {
    uint16_t len = 0;
    in.read(reinterpret_cast<char*>(&len), sizeof(len));
    if (!in || len == 0) return "";
    std::string s(len, '\0');
    in.read(&s[0], len);
    return s;
}

static const uint32_t CACHE_MAGIC = 0x4D495155; // 'MIQU'
static const uint32_t CACHE_VERSION = 3;

static bool try_load_cache(const std::string& icon_theme, const std::vector<std::string>& dirs, std::vector<DesktopApp>& out_apps) {
    std::string path = get_app_cache_path(icon_theme);
    if (path.empty() || !fs::exists(path)) return false;

    std::ifstream in(path, std::ios::binary);
    if (!in.is_open()) return false;

    uint32_t magic = 0, version = 0;
    in.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    in.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (magic != CACHE_MAGIC || version != CACHE_VERSION) return false;

    std::string cached_theme = read_str(in);
    if (cached_theme != icon_theme) return false;

    uint32_t dir_count = 0;
    in.read(reinterpret_cast<char*>(&dir_count), sizeof(dir_count));
    if (dir_count != static_cast<uint32_t>(dirs.size())) return false;

    for (uint32_t i = 0; i < dir_count; ++i) {
        std::string cached_dir = read_str(in);
        int64_t cached_mtime = 0;
        in.read(reinterpret_cast<char*>(&cached_mtime), sizeof(cached_mtime));

        if (cached_dir != dirs[i]) return false;
        int64_t current_mtime = get_dir_mtime(dirs[i]);
        if (cached_mtime != current_mtime) return false;
    }

    uint32_t app_count = 0;
    in.read(reinterpret_cast<char*>(&app_count), sizeof(app_count));
    if (!in) return false;

    out_apps.clear();
    out_apps.reserve(app_count);

    for (uint32_t i = 0; i < app_count; ++i) {
        DesktopApp app;
        app.id = read_str(in);
        app.name = read_str(in);
        app.generic_name = read_str(in);
        app.comment = read_str(in);
        app.exec_cmd = read_str(in);
        app.icon_name = read_str(in);
        app.icon_path = read_str(in);
        app.categories = read_str(in);
        app.keywords = read_str(in);
        uint8_t flags = 0;
        in.read(reinterpret_cast<char*>(&flags), sizeof(flags));
        app.terminal = (flags & 1) != 0;
        app.no_display = (flags & 2) != 0;

        if (!in) {
            out_apps.clear();
            return false;
        }
        out_apps.push_back(std::move(app));
    }

    return true;
}

static void save_cache(const std::string& icon_theme, const std::vector<std::string>& dirs, const std::vector<DesktopApp>& apps) {
    std::string path = get_app_cache_path(icon_theme);
    if (path.empty()) return;

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out.is_open()) return;

    out.write(reinterpret_cast<const char*>(&CACHE_MAGIC), sizeof(CACHE_MAGIC));
    out.write(reinterpret_cast<const char*>(&CACHE_VERSION), sizeof(CACHE_VERSION));
    write_str(out, icon_theme);

    uint32_t dir_count = static_cast<uint32_t>(dirs.size());
    out.write(reinterpret_cast<const char*>(&dir_count), sizeof(dir_count));

    for (const auto& dir : dirs) {
        write_str(out, dir);
        int64_t mtime = get_dir_mtime(dir);
        out.write(reinterpret_cast<const char*>(&mtime), sizeof(mtime));
    }

    uint32_t app_count = static_cast<uint32_t>(apps.size());
    out.write(reinterpret_cast<const char*>(&app_count), sizeof(app_count));

    for (const auto& app : apps) {
        write_str(out, app.id);
        write_str(out, app.name);
        write_str(out, app.generic_name);
        write_str(out, app.comment);
        write_str(out, app.exec_cmd);
        write_str(out, app.icon_name);
        write_str(out, app.icon_path);
        write_str(out, app.categories);
        write_str(out, app.keywords);
        uint8_t flags = (app.terminal ? 1 : 0) | (app.no_display ? 2 : 0);
        out.write(reinterpret_cast<const char*>(&flags), sizeof(flags));
    }
}

AppManager::AppManager() {
    Config::get()->add_change_listener([this]() {
        invalidate();
    });
}

AppManager* AppManager::get() {
    static AppManager s_instance;
    return &s_instance;
}

void AppManager::invalidate() {
    m_scanned = false;
    m_apps.clear();
}

std::string AppManager::clean_exec(const std::string& raw) {
    std::string result;
    result.reserve(raw.size());

    size_t i = 0;
    while (i < raw.size()) {
        if (raw[i] == '%' && i + 1 < raw.size() && raw[i + 1] == '%') {
            result += '%';
            i += 2;
            continue;
        }

        if (raw[i] == '%' && i + 1 < raw.size()) {
            char code = raw[i + 1];
            if (code == 'f' || code == 'F' || code == 'u' || code == 'U' ||
                code == 'd' || code == 'D' || code == 'n' || code == 'N' ||
                code == 'i' || code == 'c' || code == 'k' || code == 'v' ||
                code == 'm') {
                
                if (!result.empty() && result.back() == ' ') {
                    if (i + 2 == raw.size() || raw[i + 2] == ' ' || raw[i + 2] == '"' || raw[i + 2] == '\'') {
                        result.pop_back();
                    }
                }
                
                if (result.size() >= 1 && (result.back() == '"' || result.back() == '\'')) {
                    char quote = result.back();
                    if (i + 2 < raw.size() && raw[i + 2] == quote) {
                        result.pop_back();
                        if (!result.empty() && result.back() == ' ') {
                            result.pop_back();
                        }
                        i += 3;
                        continue;
                    }
                }

                i += 2;
                continue;
            }
        }

        result += raw[i++];
    }

    while (!result.empty() && (result.back() == ' ' || result.back() == '\t')) {
        result.pop_back();
    }

    return result;
}

void AppManager::rescan_internal() {
    m_apps.clear();

    const char* home = getenv("HOME");
    std::string home_str = home ? home : "";

    std::vector<std::string> raw_dirs;

    const char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && *xdg_data_home) {
        raw_dirs.push_back(std::string(xdg_data_home) + "/applications");
    } else if (!home_str.empty()) {
        raw_dirs.push_back(home_str + "/.local/share/applications");
    }

    if (!home_str.empty()) {
        raw_dirs.push_back(home_str + "/.local/share/flatpak/exports/share/applications");
    }

    const char* xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs && *xdg_data_dirs) {
        std::stringstream ss(xdg_data_dirs);
        std::string dir;
        while (std::getline(ss, dir, ':')) {
            if (!dir.empty()) {
                raw_dirs.push_back(dir + "/applications");
            }
        }
    }

    raw_dirs.push_back("/usr/share/applications");
    raw_dirs.push_back("/usr/local/share/applications");
    raw_dirs.push_back("/var/lib/flatpak/exports/share/applications");
    raw_dirs.push_back("/var/lib/snapd/desktop/applications");

    std::vector<std::string> dirs;
    std::set<std::string> seen_dirs;
    for (const auto& d : raw_dirs) {
        if (!d.empty() && seen_dirs.find(d) == seen_dirs.end()) {
            seen_dirs.insert(d);
            dirs.push_back(d);
        }
    }

    std::string active_theme = Config::get()->metrics.icon_theme;
    if (active_theme.empty()) active_theme = "hicolor";

    // 1. Fast Path: If cache exists and directory mtimes match, load binary cache (<0.2ms)
    if (try_load_cache(active_theme, dirs, m_apps)) {
        m_scanned = true;
        return;
    }

    std::set<std::string> seen_ids;
    std::set<std::string> seen_names;

    for (const auto& dir : dirs) {
        if (!fs::exists(dir)) continue;

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(dir, ec)) {
            if (ec) break;
            if (!entry.is_regular_file() && !entry.is_symlink()) continue;
            if (entry.path().extension() != ".desktop") continue;

            std::string desktop_id = entry.path().filename().string();
            if (seen_ids.find(desktop_id) != seen_ids.end()) {
                continue;
            }

            std::ifstream file(entry.path());
            if (!file.is_open()) continue;

            std::string line;
            bool in_desktop_entry = false;
            std::string name, generic_name, comment, exec, icon, categories, keywords;
            bool no_display = false;
            bool terminal = false;

            while (std::getline(file, line)) {
                if (line.empty() || line[0] == '#') continue;

                if (line[0] == '[') {
                    in_desktop_entry = (line == "[Desktop Entry]");
                    continue;
                }

                if (!in_desktop_entry) continue;

                auto eq = line.find('=');
                if (eq == std::string::npos) continue;

                std::string key = line.substr(0, eq);
                std::string val = line.substr(eq + 1);

                if (key == "Name" && name.empty()) name = val;
                else if (key == "GenericName" && generic_name.empty()) generic_name = val;
                else if (key == "Comment" && comment.empty()) comment = val;
                else if (key == "Exec" && exec.empty()) exec = val;
                else if (key == "Icon" && icon.empty()) icon = val;
                else if (key == "Categories" && categories.empty()) categories = val;
                else if (key == "Keywords" && keywords.empty()) keywords = val;
                else if (key == "NoDisplay" && val == "true") no_display = true;
                else if (key == "Terminal" && val == "true") terminal = true;
            }

            seen_ids.insert(desktop_id);

            if (no_display || name.empty() || exec.empty()) {
                continue;
            }

            if (seen_names.find(name) != seen_names.end()) {
                continue;
            }
            seen_names.insert(name);

            // Pre-resolve icon path so ImageView doesn't need to search disk directories on startup
            std::string resolved_icon;
            if (!icon.empty()) {
                if (icon[0] == '/' || icon[0] == '.' || icon[0] == '~') {
                    resolved_icon = icon;
                } else {
                    resolved_icon = ImageView::resolve_icon_path(icon);
                }
            }
            std::string clean_command = clean_exec(exec);

            DesktopApp app;
            app.id = desktop_id;
            app.name = name;
            app.generic_name = generic_name;
            app.comment = comment;
            app.icon_name = icon;
            app.icon_path = resolved_icon;
            app.exec_cmd = clean_command;
            app.categories = categories;
            app.keywords = keywords;
            app.terminal = terminal;
            app.no_display = no_display;

            m_apps.push_back(std::move(app));
        }
    }

    std::sort(m_apps.begin(), m_apps.end(), [](const DesktopApp& a, const DesktopApp& b) {
        return a.name < b.name;
    });

    // 2. Save binary cache for subsequent instant launches
    save_cache(active_theme, dirs, m_apps);

    m_scanned = true;
}

const std::vector<DesktopApp>& AppManager::get_installed_apps(bool force_rescan) {
    if (!m_scanned || force_rescan) {
        rescan_internal();
    }
    return m_apps;
}

void AppManager::scan_async(std::function<void(const std::vector<DesktopApp>&)> callback) {
    ThreadPool::get().enqueue([this, callback]() {
        rescan_internal();
        if (callback) {
            if (AppEngine::instance()) {
                AppEngine::instance()->post([this, callback]() {
                    callback(this->m_apps);
                });
            } else {
                callback(this->m_apps);
            }
        }
    });
}

void AppManager::launch(const DesktopApp& app) {
    if (app.exec_cmd.empty()) return;
    launch_command(app.exec_cmd, app.terminal);
}

void AppManager::launch_command(const std::string& cmd, bool terminal) {
    if (cmd.empty()) return;

    std::string full_cmd = cmd;
    if (terminal) {
        const char* env_term = getenv("TERMINAL");
        std::string term = (env_term && *env_term) ? env_term : "kitty || foot || alacritty || wezterm || weston-terminal || xterm";
        full_cmd = term + " -e " + cmd;
    }

    pid_t pid = fork();
    if (pid == 0) {
        setsid();
        pid_t grand_child = fork();
        if (grand_child == 0) {
            execl("/bin/sh", "sh", "-c", full_cmd.c_str(), nullptr);
            _exit(1);
        }
        _exit(0);
    } else if (pid > 0) {
        waitpid(pid, nullptr, 0);
    }
}

} // namespace miqu
