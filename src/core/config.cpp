#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/fs_utils.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>
#include <algorithm>

namespace miqu {

namespace fs = std::filesystem;

static std::shared_ptr<Config> s_instance = nullptr;

static std::string trim_str(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n\"'");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n\"'");
    return str.substr(first, (last - first + 1));
}

static std::string expand_path(const std::string& path) {
    if (path.empty()) return path;

    std::string result = path;

    // 1. Expand ~ or ~username
    if (result[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            result = std::string(home) + result.substr(1);
        }
    }

    // 2. Expand $VAR and ${VAR}
    size_t pos = 0;
    while ((pos = result.find('$', pos)) != std::string::npos) {
        size_t end = pos + 1;
        std::string var_name;
        if (end < result.size() && result[end] == '{') {
            size_t close_brace = result.find('}', end);
            if (close_brace != std::string::npos) {
                var_name = result.substr(end + 1, close_brace - end - 1);
                const char* val = getenv(var_name.c_str());
                std::string rep = val ? val : "";
                result.replace(pos, close_brace - pos + 1, rep);
                pos += rep.size();
                continue;
            }
        } else {
            while (end < result.size() && (std::isalnum(result[end]) || result[end] == '_')) {
                end++;
            }
            var_name = result.substr(pos + 1, end - pos - 1);
            if (!var_name.empty()) {
                const char* val = getenv(var_name.c_str());
                std::string rep = val ? val : "";
                result.replace(pos, end - pos, rep);
                pos += rep.size();
                continue;
            }
        }
        pos++;
    }

    return result;
}

std::shared_ptr<Config> Config::get() {
    if (!s_instance) {
        s_instance = std::make_shared<Config>();
        s_instance->init_toolkit_defaults();
    }
    return s_instance;
}

static void load_config_file_internal(const std::string& path, Config::Colors& colors, Config::Metrics& metrics, std::vector<std::string>& loaded_files, int depth) {
    if (depth > 10) return;

    std::string expanded = expand_path(path);
    if (!fs::exists(expanded)) return;

    std::string canonical_path;
    try {
        canonical_path = fs::canonical(expanded).string();
    } catch (...) {
        canonical_path = fs::absolute(expanded).string();
    }

    if (std::find(loaded_files.begin(), loaded_files.end(), canonical_path) == loaded_files.end()) {
        loaded_files.push_back(canonical_path);
    }

    std::ifstream file(expanded);
    if (!file.is_open()) return;

    fs::path parent_dir = fs::path(expanded).parent_path();
    const char* home_env = getenv("HOME");
    std::string config_base = home_env ? (std::string(home_env) + "/.config/miquland") : "";

    std::string line;
    std::string current_section = "";

    while (std::getline(file, line)) {
        line = trim_str(line);
        if (line.empty() || line[0] == '#' || line[0] == ';') continue;

        // Section header [section]
        if (line.front() == '[' && line.back() == ']') {
            current_section = trim_str(line.substr(1, line.size() - 2));
            std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
            continue;
        }

        // Section header section { ... }
        if (line.back() == '{') {
            current_section = trim_str(line.substr(0, line.size() - 1));
            std::transform(current_section.begin(), current_section.end(), current_section.begin(), ::tolower);
            continue;
        }
        if (line == "}") {
            current_section = "";
            continue;
        }

        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = trim_str(line.substr(0, eq_pos));
        std::string val = trim_str(line.substr(eq_pos + 1));
        std::transform(key.begin(), key.end(), key.begin(), ::tolower);

        // Strip inline comments from values
        size_t c_pos = std::string::npos;
        if (!val.empty() && val[0] == '#') {
            size_t space_pos = val.find_first_of(" \t");
            if (space_pos != std::string::npos) {
                c_pos = val.find('#', space_pos);
            }
        } else {
            c_pos = val.find('#');
        }

        if (c_pos != std::string::npos) {
            val = trim_str(val.substr(0, c_pos));
        }

        if (key == "source" || key == "include") {
            std::string inc_path = expand_path(val);
            if (!fs::exists(inc_path)) {
                // Try relative to current file directory
                fs::path rel1 = parent_dir / val;
                if (fs::exists(rel1)) {
                    inc_path = rel1.string();
                } else if (!config_base.empty()) {
                    // Try relative to ~/.config/miquland/
                    fs::path rel2 = fs::path(config_base) / val;
                    if (fs::exists(rel2)) {
                        inc_path = rel2.string();
                    }
                }
            }

            // Always track target file so inotify watches its directory even if file is created later
            std::string abs_inc;
            try {
                abs_inc = fs::absolute(inc_path).string();
            } catch (...) {
                abs_inc = inc_path;
            }
            if (std::find(loaded_files.begin(), loaded_files.end(), abs_inc) == loaded_files.end()) {
                loaded_files.push_back(abs_inc);
            }

            load_config_file_internal(inc_path, colors, metrics, loaded_files, depth + 1);
            continue;
        }

        // Color mappings
        if (key == "color_background" || key == "background" || key == "bg_color" || key == "col.background") {
            colors.background = Color::from_hex(val, colors.background);
        } else if (key == "color_on_background" || key == "on_background" || key == "col.on_background") {
            colors.on_background = Color::from_hex(val, colors.on_background);
        } else if (key == "color_surface" || key == "surface" || key == "menu_bg" || key == "col.surface") {
            colors.surface = Color::from_hex(val, colors.surface);
        } else if (key == "color_on_surface" || key == "on_surface" || key == "text_color" || key == "text" || key == "col.on_surface" || key == "col.text") {
            colors.on_surface = Color::from_hex(val, colors.on_surface);
        } else if (key == "color_surface_variant" || key == "surface_variant" || key == "col.surface_variant") {
            colors.surface_variant = Color::from_hex(val, colors.surface_variant);
        } else if (key == "color_on_surface_variant" || key == "on_surface_variant" || key == "text_muted" || key == "col.on_surface_variant") {
            colors.on_surface_variant = Color::from_hex(val, colors.on_surface_variant);
        } else if (key == "color_primary" || key == "primary" || key == "accent" || key == "window_border_color_active" || key == "border_color_active" || key == "col.primary" || key == "col.active_border") {
            colors.primary = Color::from_hex(val, colors.primary);
        } else if (key == "color_on_primary" || key == "on_primary" || key == "col.on_primary") {
            colors.on_primary = Color::from_hex(val, colors.on_primary);
        } else if (key == "color_primary_container" || key == "primary_container" || key == "col.primary_container") {
            colors.primary_container = Color::from_hex(val, colors.primary_container);
        } else if (key == "color_on_primary_container" || key == "on_primary_container" || key == "col.on_primary_container") {
            colors.on_primary_container = Color::from_hex(val, colors.on_primary_container);
        } else if (key == "color_outline" || key == "outline" || key == "border" || key == "window_border_color_inactive" || key == "border_color_inactive" || key == "col.outline" || key == "col.inactive_border") {
            colors.outline = Color::from_hex(val, colors.outline);
        } else if (key == "color_outline_variant" || key == "outline_variant" || key == "col.outline_variant") {
            colors.outline_variant = Color::from_hex(val, colors.outline_variant);
        } else if (key == "color_backdrop" || key == "backdrop" || key == "col.backdrop") {
            colors.backdrop = Color::from_hex(val, colors.backdrop);
        } else if (key == "icon_theme") {
            metrics.icon_theme = val;
        } else if (key == "font_family" || key == "font") {
            metrics.font_family = val;
        } else if (key == "font_size") {
            try { metrics.font_size = std::stoi(val); } catch (...) {}
        } else if (key == "h1_size" || key == "heading1_size") {
            try { metrics.h1_size = std::stoi(val); } catch (...) {}
        } else if (key == "h2_size" || key == "heading2_size") {
            try { metrics.h2_size = std::stoi(val); } catch (...) {}
        } else if (key == "h3_size" || key == "heading3_size") {
            try { metrics.h3_size = std::stoi(val); } catch (...) {}
        } else if (key == "caption_size") {
            try { metrics.caption_size = std::stoi(val); } catch (...) {}
        } else if (key == "window_border_radius" || key == "border_radius" || key == "corner_radius" || key == "rounding") {
            try { metrics.corner_radius = std::stoi(val); } catch (...) {}
        } else if (key == "window_border_width" || key == "border_width" || key == "border_size") {
            try { metrics.border_width = std::stoi(val); } catch (...) {}
        }
    }
}

bool Config::load_from_file(const std::string& path) {
    std::string expanded = expand_path(path);
    if (!fs::exists(expanded)) return false;

    if (path.find("miqutoolkit.conf") == std::string::npos) {
        m_app_config_file = expanded;
    }

    load_config_file_internal(expanded, colors, metrics, m_loaded_files, 0);
    notify_changed();
    return true;
}

void Config::notify_changed() {
    ImageView::clear_cache();
    for (auto& listener : m_change_listeners) {
        if (listener) listener();
    }
}

void Config::init_toolkit_defaults() {
    m_loaded_files.clear();
    colors = Colors{};
    metrics = Metrics{};

    std::string user_cfg_dir = FsUtils::get_user_config_dir("miqutoolkit");
    std::string user_cfg_file = user_cfg_dir.empty() ? "" : (user_cfg_dir + "/miqutoolkit.conf");

    bool loaded_defaults = false;

    // Tier 2: Check if user config exists
    if (!user_cfg_file.empty() && fs::exists(user_cfg_file)) {
        std::string exp = expand_path(user_cfg_file);
        load_config_file_internal(exp, colors, metrics, m_loaded_files, 0);
        loaded_defaults = true;
    }

    if (!loaded_defaults) {
        // Tier 3: Safe root fallback (Dead End Authority)
        const std::vector<std::string> root_candidates = {
            "/usr/share/miqutoolkit/miqutoolkit.conf",
            "/etc/xdg/miqutoolkit/miqutoolkit.conf",
            "/etc/miqutoolkit/miqutoolkit.conf",
            "/usr/local/share/miqutoolkit/miqutoolkit.conf",
            "assets/miqutoolkit.conf",
            "../assets/miqutoolkit.conf"
        };

        for (const auto& root_file : root_candidates) {
            if (fs::exists(root_file)) {
                std::string exp = expand_path(root_file);
                load_config_file_internal(exp, colors, metrics, m_loaded_files, 0);
                loaded_defaults = true;
                break;
            }
        }
    }

    // Re-apply app-specific config file on top of defaults if one was registered
    if (!m_app_config_file.empty() && fs::exists(m_app_config_file)) {
        load_config_file_internal(m_app_config_file, colors, metrics, m_loaded_files, 0);
    }

    notify_changed();
}

std::string Config::init_user_config(
    const std::string& app_name,
    const std::string& main_file,
    const std::vector<std::string>& additional_files
) {
    if (app_name.empty()) return "";

    std::string primary = main_file.empty() ? (app_name + ".conf") : main_file;

    // 1. Resolve user config directory
    std::string user_cfg_dir;
    const char* xdg_config = getenv("XDG_CONFIG_HOME");
    if (xdg_config && *xdg_config) {
        user_cfg_dir = std::string(xdg_config) + "/" + app_name;
    } else {
        const char* home = getenv("HOME");
        if (home && *home) {
            user_cfg_dir = std::string(home) + "/.config/" + app_name;
        }
    }
    if (user_cfg_dir.empty()) return "";

    std::error_code ec;
    fs::create_directories(user_cfg_dir, ec);

    std::vector<std::string> all_files = {primary};
    all_files.insert(all_files.end(), additional_files.begin(), additional_files.end());

    for (const auto& fname : all_files) {
        fs::path dest = fs::path(user_cfg_dir) / fname;
        if (fs::exists(dest)) {
            continue; // File already present in user config, keep user customizations
        }

        std::vector<std::string> candidates = {
            "/usr/share/" + app_name + "/" + fname,
            "/etc/xdg/" + app_name + "/" + fname,
            "/etc/" + app_name + "/" + fname,
            "/usr/local/share/" + app_name + "/" + fname,
            "assets/" + fname,
            "../assets/" + fname
        };

        for (const auto& cand : candidates) {
            if (fs::exists(cand)) {
                fs::copy_file(cand, dest, fs::copy_options::overwrite_existing, ec);
                if (!ec) {
                    std::cout << "[" << app_name << "] Initialized default configuration: copied "
                              << cand << " to " << dest.string() << "\n";
                    break;
                }
            }
        }
    }

    fs::path primary_dest = fs::path(user_cfg_dir) / primary;
    if (fs::exists(primary_dest)) {
        return primary_dest.string();
    }
    return "";
}

std::string FsUtils::init_user_config(
    const std::string& app_name,
    const std::string& main_file,
    const std::vector<std::string>& additional_files
) {
    return Config::init_user_config(app_name, main_file, additional_files);
}

} // namespace miqu
