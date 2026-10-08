#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/thread_pool.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <set>
#include <unordered_set>
#include <unordered_map>
#include <list>
#include "miqutoolkit/core/string_utils.hpp"
#include <mutex>
#include <algorithm>
#include <iostream>
#include <cmath>
#include <unistd.h>

namespace miqu {

namespace fs = std::filesystem;

static constexpr size_t MAX_SURFACE_CACHE_ENTRIES = 64;
static std::mutex s_cache_mutex;
static std::list<std::pair<std::string, cairo_surface_t*>> s_surface_lru;
static std::unordered_map<std::string, std::list<std::pair<std::string, cairo_surface_t*>>::iterator> s_surface_cache;
static std::unordered_map<std::string, std::string> s_path_cache;
static std::unordered_map<std::string, std::pair<int, int>> s_dimension_cache;

static std::mutex s_pending_mutex;
static std::unordered_set<std::string> s_pending_thumbnails;

static std::mutex s_theme_hierarchy_mutex;
static std::map<std::string, std::vector<std::string>> s_cached_theme_search_dirs;

static std::string trim_str(const std::string& str) {
    return StringUtils::trim(str, " \t\r\n\"'");
}

static std::string str_to_lower(std::string s) {
    return StringUtils::to_lower(std::move(s));
}

void ImageView::clear_cache() {
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        for (auto& item : s_surface_lru) {
            if (item.second) {
                cairo_surface_destroy(item.second);
            }
        }
        s_surface_lru.clear();
        s_surface_cache.clear();
        s_path_cache.clear();
        s_dimension_cache.clear();
    }
    {
        std::lock_guard<std::mutex> lock(s_theme_hierarchy_mutex);
        s_cached_theme_search_dirs.clear();
    }
}

static bool get_image_dimensions(const std::string& file_path, int& out_w, int& out_h) {
    if (file_path.empty()) return false;
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_dimension_cache.find(file_path);
        if (it != s_dimension_cache.end()) {
            out_w = it->second.first;
            out_h = it->second.second;
            return true;
        }
    }

    int w = 0, h = 0;
    if (gdk_pixbuf_get_file_info(file_path.c_str(), &w, &h) && w > 0 && h > 0) {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        s_dimension_cache[file_path] = {w, h};
        out_w = w;
        out_h = h;
        return true;
    }
    return false;
}

static std::vector<std::string> get_icon_base_roots() {
    std::vector<std::string> roots;
    const char* home = getenv("HOME");
    std::string home_str = home ? home : "";

    const char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home && *xdg_data_home) {
        roots.push_back(std::string(xdg_data_home) + "/icons");
    } else if (!home_str.empty()) {
        roots.push_back(home_str + "/.local/share/icons");
    }

    if (!home_str.empty()) {
        roots.push_back(home_str + "/.icons");
    }

    const char* xdg_data_dirs = getenv("XDG_DATA_DIRS");
    if (xdg_data_dirs && *xdg_data_dirs) {
        std::stringstream ss(xdg_data_dirs);
        std::string dir;
        while (std::getline(ss, dir, ':')) {
            dir = trim_str(dir);
            if (!dir.empty()) {
                roots.push_back(dir + "/icons");
            }
        }
    }

    roots.push_back("/usr/share/icons");
    roots.push_back("/usr/local/share/icons");
    roots.push_back("/var/lib/flatpak/exports/share/icons");

    std::vector<std::string> unique_roots;
    std::set<std::string> seen;
    for (const auto& r : roots) {
        if (seen.find(r) == seen.end() && fs::exists(r)) {
            seen.insert(r);
            unique_roots.push_back(r);
        }
    }
    return unique_roots;
}

static std::vector<std::string> get_all_theme_dirs(const std::string& theme_name, const std::vector<std::string>& base_roots) {
    std::vector<std::string> result;
    if (theme_name.empty()) return result;

    std::string lower_theme = str_to_lower(theme_name);

    for (const auto& root : base_roots) {
        if (!fs::exists(root)) continue;

        std::string exact = root + "/" + theme_name;
        if (fs::exists(exact) && fs::is_directory(exact)) {
            result.push_back(exact);
            continue;
        }

        std::error_code ec;
        for (const auto& entry : fs::directory_iterator(root, ec)) {
            if (entry.is_directory()) {
                if (str_to_lower(entry.path().filename().string()) == lower_theme) {
                    result.push_back(entry.path().string());
                }
            }
        }
    }
    return result;
}

static void parse_index_theme(const std::string& theme_dir,
                              std::vector<std::string>& out_inherits,
                              std::vector<std::string>& out_subdirs) {
    std::string path = theme_dir + "/index.theme";
    if (!fs::exists(path)) return;

    std::ifstream file(path);
    if (!file.is_open()) return;

    std::string line;
    bool in_section = false;

    while (std::getline(file, line)) {
        line = trim_str(line);
        if (line.empty() || line[0] == '#') continue;

        if (line[0] == '[') {
            in_section = (line == "[Icon Theme]");
            continue;
        }

        if (!in_section) continue;

        auto eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = trim_str(line.substr(0, eq));
        std::string val = trim_str(line.substr(eq + 1));

        if (key == "Inherits") {
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item = trim_str(item);
                if (!item.empty()) out_inherits.push_back(item);
            }
        } else if (key == "Directories" || key == "ScaledDirectories") {
            std::stringstream ss(val);
            std::string item;
            while (std::getline(ss, item, ',')) {
                item = trim_str(item);
                if (!item.empty()) out_subdirs.push_back(item);
            }
        }
    }
}

std::string ImageView::resolve_icon_path(const std::string& icon_name) {
    if (icon_name.empty()) return "";

    std::string path_str = icon_name;
    if (path_str[0] == '~') {
        const char* home = getenv("HOME");
        if (home) {
            path_str = std::string(home) + path_str.substr(1);
        }
    }

    if ((path_str[0] == '/' || path_str[0] == '.') && fs::exists(path_str)) {
        return path_str;
    }

    std::string user_theme = Config::get()->metrics.icon_theme;
    if (user_theme.empty()) user_theme = "hicolor";

    std::string cache_key = user_theme + ":" + icon_name;
    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_path_cache.find(cache_key);
        if (it != s_path_cache.end()) {
            return it->second;
        }
    }

    std::vector<std::string> search_dirs_to_check;
    {
        std::lock_guard<std::mutex> lock(s_theme_hierarchy_mutex);
        auto th_it = s_cached_theme_search_dirs.find(user_theme);
        if (th_it != s_cached_theme_search_dirs.end()) {
            search_dirs_to_check = th_it->second;
        } else {
            auto base_roots = get_icon_base_roots();
            std::vector<std::string> theme_search_list;
            std::set<std::string> visited_themes;

            auto add_theme = [&](const std::string& t) {
                if (t.empty()) return;
                std::string lt = str_to_lower(t);
                if (visited_themes.find(lt) == visited_themes.end()) {
                    visited_themes.insert(lt);
                    theme_search_list.push_back(t);
                }
            };

            add_theme(user_theme);

            for (size_t i = 0; i < theme_search_list.size(); ++i) {
                std::string t = theme_search_list[i];
                auto dirs = get_all_theme_dirs(t, base_roots);
                for (const auto& t_dir : dirs) {
                    std::vector<std::string> inherits, subdirs;
                    parse_index_theme(t_dir, inherits, subdirs);
                    for (const auto& inh : inherits) {
                        add_theme(inh);
                    }
                }
            }

            add_theme("Papirus");
            add_theme("Papirus-Dark");
            add_theme("hicolor");
            add_theme("Adwaita");
            add_theme("breeze");

            const std::vector<std::string> fallback_subdirs = {
                "scalable/apps", "48x48/apps", "64x64/apps", "32x32/apps", "128x128/apps", "256x256/apps", "512x512/apps",
                "24x24/apps", "22x22/apps", "16x16/apps", "16/apps", "22/apps", "24/apps", "32/apps", "48/apps", "64/apps", "128/apps", "apps",
                "scalable/categories", "48x48/categories", "scalable/devices", "48x48/devices",
                "scalable/places", "48x48/places", "scalable/mimetypes", "48x48/mimetypes",
                "scalable/actions", "48x48/actions", "scalable/status", "48x48/status",
                "symbolic/apps", "symbolic",
                "48x48", "64x64", "32x32", "scalable", ""
            };

            std::set<std::string> existing_dirs_seen;
            for (const auto& th : theme_search_list) {
                auto theme_dirs = get_all_theme_dirs(th, base_roots);
                if (theme_dirs.empty()) continue;

                for (const auto& theme_dir : theme_dirs) {
                    std::vector<std::string> inherits, theme_subdirs;
                    parse_index_theme(theme_dir, inherits, theme_subdirs);

                    std::vector<std::string> subdirs_to_check = fallback_subdirs;
                    for (const auto& d : theme_subdirs) {
                        if (std::find(subdirs_to_check.begin(), subdirs_to_check.end(), d) == subdirs_to_check.end()) {
                            subdirs_to_check.push_back(d);
                        }
                    }

                    for (const auto& sub : subdirs_to_check) {
                        std::string search_dir = sub.empty() ? theme_dir : (theme_dir + "/" + sub);
                        if (existing_dirs_seen.find(search_dir) == existing_dirs_seen.end()) {
                            if (fs::exists(search_dir)) {
                                existing_dirs_seen.insert(search_dir);
                                search_dirs_to_check.push_back(search_dir);
                            }
                        }
                    }
                }
            }

            s_cached_theme_search_dirs[user_theme] = search_dirs_to_check;
        }
    }

    std::string clean_name = icon_name;
    for (const char* ext : {".svg", ".png", ".xpm", ".jpg", ".jpeg", ".ico"}) {
        std::string ext_str(ext);
        if (clean_name.size() > ext_str.size() &&
            clean_name.substr(clean_name.size() - ext_str.size()) == ext_str) {
            clean_name = clean_name.substr(0, clean_name.size() - ext_str.size());
            break;
        }
    }

    std::vector<std::string> name_candidates;
    name_candidates.push_back(clean_name);
    if (str_to_lower(clean_name) != clean_name) {
        name_candidates.push_back(str_to_lower(clean_name));
    }

    // For reverse-DNS identifiers (e.g. org.gnome.Nautilus, org.xfce.thunar, io.github.alacritty)
    size_t last_dot = clean_name.rfind('.');
    if (last_dot != std::string::npos && last_dot + 1 < clean_name.size()) {
        std::string base_id = clean_name.substr(last_dot + 1);
        name_candidates.push_back(base_id);
        if (str_to_lower(base_id) != base_id) {
            name_candidates.push_back(str_to_lower(base_id));
        }
    }

    if (clean_name.size() > 9 && clean_name.substr(clean_name.size() - 9) == "-symbolic") {
        name_candidates.push_back(clean_name.substr(0, clean_name.size() - 9));
    } else {
        name_candidates.push_back(clean_name + "-symbolic");
    }

    const std::vector<std::string> exts = { ".svg", ".png", ".xpm", ".jpg", "" };

    for (const auto& search_dir : search_dirs_to_check) {
        for (const auto& name_cand : name_candidates) {
            for (const auto& ext : exts) {
                std::string candidate = search_dir + "/" + name_cand + ext;
                if (fs::exists(candidate) && !fs::is_directory(candidate)) {
                    std::lock_guard<std::mutex> lock(s_cache_mutex);
                    s_path_cache[cache_key] = candidate;
                    return candidate;
                }
            }
        }
    }

    // Pixmap directory fallback
    const char* home = getenv("HOME");
    std::string home_str = home ? home : "";
    std::vector<std::string> pixmap_dirs = {
        "/usr/share/pixmaps",
        "/usr/local/share/pixmaps",
        home_str + "/.local/share/pixmaps"
    };

    for (const auto& pdir : pixmap_dirs) {
        if (!fs::exists(pdir)) continue;
        for (const auto& name_cand : name_candidates) {
            for (const auto& ext : exts) {
                std::string candidate = pdir + "/" + name_cand + ext;
                if (fs::exists(candidate) && !fs::is_directory(candidate)) {
                    s_path_cache[cache_key] = candidate;
                    return candidate;
                }
            }
        }
    }

    s_path_cache[cache_key] = "";
    return "";
}

static std::string get_thumbnail_cache_dir(bool large) {
    const char* xdg_cache = getenv("XDG_CACHE_HOME");
    std::string base;
    if (xdg_cache && *xdg_cache) {
        base = xdg_cache;
    } else {
        const char* home = getenv("HOME");
        base = home ? std::string(home) + "/.cache" : "/tmp";
    }
    std::string dir = base + "/thumbnails/" + (large ? "large" : "normal");
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

static std::string escape_uri(const std::string& path) {
    std::string out;
    out.reserve(path.size() * 3);
    for (unsigned char c : path) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
            (c >= '0' && c <= '9') || c == '/' || c == '.' || c == '_' || c == '-' || c == '~') {
            out.push_back(c);
        } else {
            char hex[4];
            snprintf(hex, sizeof(hex), "%%%02X", c);
            out.append(hex);
        }
    }
    return out;
}

static std::string get_thumbnail_file(const std::string& canonical_path, bool large) {
    std::string uri = "file://" + escape_uri(canonical_path);
    char* checksum = g_compute_checksum_for_string(G_CHECKSUM_MD5, uri.c_str(), -1);
    if (!checksum) return "";
    std::string filename = std::string(checksum) + ".png";
    g_free(checksum);
    return get_thumbnail_cache_dir(large) + "/" + filename;
}

static cairo_surface_t* pixbuf_to_cairo_surface(GdkPixbuf* pixbuf) {
    if (!pixbuf) return nullptr;

    int p_width = gdk_pixbuf_get_width(pixbuf);
    int p_height = gdk_pixbuf_get_height(pixbuf);
    int p_stride = gdk_pixbuf_get_rowstride(pixbuf);
    int p_channels = gdk_pixbuf_get_n_channels(pixbuf);
    const guchar* p_pixels = gdk_pixbuf_get_pixels(pixbuf);

    cairo_surface_t* surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, p_width, p_height);
    unsigned char* c_data = cairo_image_surface_get_data(surf);
    int c_stride = cairo_image_surface_get_stride(surf);

    cairo_surface_flush(surf);
    for (int y = 0; y < p_height; ++y) {
        const guchar* src = p_pixels + y * p_stride;
        auto* dst = reinterpret_cast<uint32_t*>(c_data + y * c_stride);
        for (int x = 0; x < p_width; ++x) {
            uint8_t r = src[0];
            uint8_t g = src[1];
            uint8_t b = src[2];
            uint8_t a = (p_channels == 4) ? src[3] : 255;
            uint8_t pr = (r * a) / 255;
            uint8_t pg = (g * a) / 255;
            uint8_t pb = (b * a) / 255;
            dst[x] = (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(pr) << 16) |
                     (static_cast<uint32_t>(pg) << 8) | static_cast<uint32_t>(pb);
            src += p_channels;
        }
    }
    cairo_surface_mark_dirty(surf);
    return surf;
}

void ImageView::preload(const std::string& source, ImageQuality quality) {
    if (source.empty() || quality != ImageQuality::ThumbnailFast) return;

    std::string resolved = ImageView::resolve_icon_path(source);
    if (resolved.empty() || !fs::exists(resolved)) return;

    std::error_code ec;
    fs::path can_path = fs::canonical(resolved, ec);
    std::string effective_path = ec ? resolved : can_path.string();
    std::string thumb_file = get_thumbnail_file(effective_path, true);
    if (thumb_file.empty()) return;

    if (fs::exists(thumb_file)) {
        std::error_code tec, oec;
        auto thumb_time = fs::last_write_time(thumb_file, tec);
        auto orig_time = fs::last_write_time(effective_path, oec);
        if (!tec && !oec && thumb_time >= orig_time) {
            return;
        }
    }

    {
        std::lock_guard<std::mutex> lock(s_pending_mutex);
        if (s_pending_thumbnails.count(effective_path) > 0) {
            return;
        }
        s_pending_thumbnails.insert(effective_path);
    }

    ThreadPool::get().enqueue([effective_path, thumb_file]() {
        GError* gen_err = nullptr;
        GdkPixbuf* gen_pix = gdk_pixbuf_new_from_file_at_scale(effective_path.c_str(), 256, 256, TRUE, &gen_err);
        if (gen_pix) {
            std::string tmp_file = thumb_file + ".tmp." + std::to_string(getpid()) + "." + std::to_string(rand());
            GError* save_err = nullptr;
            if (gdk_pixbuf_save(gen_pix, tmp_file.c_str(), "png", &save_err, "compression", "1", nullptr)) {
                rename(tmp_file.c_str(), thumb_file.c_str());
            } else {
                unlink(tmp_file.c_str());
                if (save_err) g_error_free(save_err);
            }
            g_object_unref(gen_pix);

            if (AppEngine::instance()) {
                AppEngine::instance()->post([]() {
                    if (AppEngine::instance()) {
                        AppEngine::instance()->request_redraw_all();
                    }
                });
            }
        } else {
            if (gen_err) g_error_free(gen_err);
        }

        {
            std::lock_guard<std::mutex> lock(s_pending_mutex);
            s_pending_thumbnails.erase(effective_path);
        }
    });
}

static std::string get_rendered_icon_cache_dir() {
    const char* xdg_cache = getenv("XDG_CACHE_HOME");
    std::string base;
    if (xdg_cache && *xdg_cache) {
        base = xdg_cache;
    } else {
        const char* home = getenv("HOME");
        base = home ? std::string(home) + "/.cache" : "/tmp";
    }
    std::string dir = base + "/miqutoolkit/rendered_icons";
    std::error_code ec;
    fs::create_directories(dir, ec);
    return dir;
}

static std::string get_rendered_icon_file(const std::string& key) {
    char* checksum = g_compute_checksum_for_string(G_CHECKSUM_MD5, key.c_str(), -1);
    if (!checksum) return "";
    std::string filename = std::string(checksum) + ".png";
    g_free(checksum);
    return get_rendered_icon_cache_dir() + "/" + filename;
}

static void box_blur_horizontal(const uint32_t* src, uint32_t* dst, int w, int h, int stride_words, int r) {
    float div = 1.0f / (2 * r + 1);
    for (int y = 0; y < h; ++y) {
        int row_offset = y * stride_words;
        int sum_a = 0, sum_r = 0, sum_g = 0, sum_b = 0;

        for (int i = -r; i <= r; ++i) {
            int x = std::clamp(i, 0, w - 1);
            uint32_t p = src[row_offset + x];
            sum_a += (p >> 24) & 0xff;
            sum_r += (p >> 16) & 0xff;
            sum_g += (p >> 8) & 0xff;
            sum_b += p & 0xff;
        }

        for (int x = 0; x < w; ++x) {
            dst[row_offset + x] = (static_cast<uint32_t>(sum_a * div) << 24) |
                                  (static_cast<uint32_t>(sum_r * div) << 16) |
                                  (static_cast<uint32_t>(sum_g * div) << 8) |
                                  static_cast<uint32_t>(sum_b * div);

            int next_x = std::min(w - 1, x + r + 1);
            int prev_x = std::max(0, x - r);

            uint32_t p_next = src[row_offset + next_x];
            uint32_t p_prev = src[row_offset + prev_x];

            sum_a += ((p_next >> 24) & 0xff) - ((p_prev >> 24) & 0xff);
            sum_r += ((p_next >> 16) & 0xff) - ((p_prev >> 16) & 0xff);
            sum_g += ((p_next >> 8) & 0xff) - ((p_prev >> 8) & 0xff);
            sum_b += (p_next & 0xff) - (p_prev & 0xff);
        }
    }
}

static void box_blur_vertical(const uint32_t* src, uint32_t* dst, int w, int h, int stride_words, int r) {
    float div = 1.0f / (2 * r + 1);
    for (int x = 0; x < w; ++x) {
        int sum_a = 0, sum_r = 0, sum_g = 0, sum_b = 0;

        for (int i = -r; i <= r; ++i) {
            int y = std::clamp(i, 0, h - 1);
            uint32_t p = src[y * stride_words + x];
            sum_a += (p >> 24) & 0xff;
            sum_r += (p >> 16) & 0xff;
            sum_g += (p >> 8) & 0xff;
            sum_b += p & 0xff;
        }

        for (int y = 0; y < h; ++y) {
            dst[y * stride_words + x] = (static_cast<uint32_t>(sum_a * div) << 24) |
                                        (static_cast<uint32_t>(sum_r * div) << 16) |
                                        (static_cast<uint32_t>(sum_g * div) << 8) |
                                        static_cast<uint32_t>(sum_b * div);

            int next_y = std::min(h - 1, y + r + 1);
            int prev_y = std::max(0, y - r);

            uint32_t p_next = src[next_y * stride_words + x];
            uint32_t p_prev = src[prev_y * stride_words + x];

            sum_a += ((p_next >> 24) & 0xff) - ((p_prev >> 24) & 0xff);
            sum_r += ((p_next >> 16) & 0xff) - ((p_prev >> 16) & 0xff);
            sum_g += ((p_next >> 8) & 0xff) - ((p_prev >> 8) & 0xff);
            sum_b += (p_next & 0xff) - (p_prev & 0xff);
        }
    }
}

static cairo_surface_t* apply_blur_and_dim(cairo_surface_t* orig_surf, int w, int h, int blur_radius, float dim_alpha) {
    if (!orig_surf) return nullptr;
    if (blur_radius <= 0 && dim_alpha <= 0.001f) {
        return orig_surf;
    }

    cairo_surface_t* final_surf = nullptr;

    if (blur_radius > 0) {
        // Downsample factor (scale 4x for extreme speed and natural low-pass smoothing)
        int down_w = std::max(64, w / 4);
        int down_h = std::max(64, h / 4);
        int r = std::clamp(blur_radius / 4, 1, 40);

        cairo_surface_t* small_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, down_w, down_h);
        cairo_t* small_cr = cairo_create(small_surf);
        cairo_scale(small_cr, static_cast<double>(down_w) / w, static_cast<double>(down_h) / h);
        cairo_set_source_surface(small_cr, orig_surf, 0, 0);
        cairo_pattern_set_filter(cairo_get_source(small_cr), CAIRO_FILTER_BILINEAR);
        cairo_paint(small_cr);
        cairo_destroy(small_cr);
        cairo_surface_flush(small_surf);

        // Apply 3-pass box blur on small buffer
        int stride_bytes = cairo_image_surface_get_stride(small_surf);
        int stride_words = stride_bytes / sizeof(uint32_t);
        uint32_t* pixels = reinterpret_cast<uint32_t*>(cairo_image_surface_get_data(small_surf));

        std::vector<uint32_t> temp_buf(stride_words * down_h);
        for (int pass = 0; pass < 3; ++pass) {
            box_blur_horizontal(pixels, temp_buf.data(), down_w, down_h, stride_words, r);
            box_blur_vertical(temp_buf.data(), pixels, down_w, down_h, stride_words, r);
        }
        cairo_surface_mark_dirty(small_surf);

        // Upsample to target dimensions
        final_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, w, h);
        cairo_t* up_cr = cairo_create(final_surf);
        cairo_scale(up_cr, static_cast<double>(w) / down_w, static_cast<double>(h) / down_h);
        cairo_set_source_surface(up_cr, small_surf, 0, 0);
        cairo_pattern_set_filter(cairo_get_source(up_cr), CAIRO_FILTER_BILINEAR);
        cairo_paint(up_cr);
        cairo_surface_destroy(small_surf);

        if (dim_alpha > 0.001f) {
            cairo_set_source_rgba(up_cr, 0.0, 0.0, 0.0, std::clamp(dim_alpha, 0.0f, 1.0f));
            cairo_paint(up_cr);
        }
        cairo_destroy(up_cr);
        cairo_surface_flush(final_surf);

        cairo_surface_destroy(orig_surf);
    } else {
        // Only dimming requested without blur
        final_surf = orig_surf;
        cairo_t* dim_cr = cairo_create(final_surf);
        cairo_set_source_rgba(dim_cr, 0.0, 0.0, 0.0, std::clamp(dim_alpha, 0.0f, 1.0f));
        cairo_paint(dim_cr);
        cairo_destroy(dim_cr);
        cairo_surface_flush(final_surf);
    }

    return final_surf;
}

static cairo_surface_t* load_surface(const std::string& path_or_name, int box_w, int box_h, FitMode fit_mode, int target_size, ImageQuality quality, int blur_radius = 0, float dim_alpha = 0.0f) {
    if (path_or_name.empty() || box_w <= 0 || box_h <= 0) return nullptr;

    std::string resolved = ImageView::resolve_icon_path(path_or_name);
    if (resolved.empty() || !fs::exists(resolved)) {
        return nullptr;
    }

    std::string file_to_decode = resolved;
    if (quality == ImageQuality::ThumbnailFast) {
        std::error_code ec;
        fs::path can_path = fs::canonical(resolved, ec);
        std::string effective_path = ec ? resolved : can_path.string();
        std::string thumb_file = get_thumbnail_file(effective_path, true); // FreeDesktop 'large' 256x256

        if (!thumb_file.empty()) {
            bool thumb_valid = false;
            if (fs::exists(thumb_file)) {
                std::error_code tec, oec;
                auto thumb_time = fs::last_write_time(thumb_file, tec);
                auto orig_time = fs::last_write_time(effective_path, oec);
                if (!tec && !oec && thumb_time >= orig_time) {
                    thumb_valid = true;
                }
            }

            if (thumb_valid) {
                file_to_decode = thumb_file;
            } else {
                // Cold thumbnail miss: Asynchronously decode on thread pool without freezing UI!
                {
                    std::lock_guard<std::mutex> lock(s_pending_mutex);
                    if (s_pending_thumbnails.count(effective_path) == 0) {
                        s_pending_thumbnails.insert(effective_path);

                        ThreadPool::get().enqueue([effective_path, thumb_file]() {
                            GError* gen_err = nullptr;
                            GdkPixbuf* gen_pix = gdk_pixbuf_new_from_file_at_scale(effective_path.c_str(), 256, 256, TRUE, &gen_err);
                            if (gen_pix) {
                                std::string tmp_file = thumb_file + ".tmp." + std::to_string(getpid()) + "." + std::to_string(rand());
                                GError* save_err = nullptr;
                                if (gdk_pixbuf_save(gen_pix, tmp_file.c_str(), "png", &save_err, "compression", "1", nullptr)) {
                                    rename(tmp_file.c_str(), thumb_file.c_str());
                                } else {
                                    unlink(tmp_file.c_str());
                                    if (save_err) g_error_free(save_err);
                                }
                                g_object_unref(gen_pix);

                                if (AppEngine::instance()) {
                                    AppEngine::instance()->post([]() {
                                        if (AppEngine::instance()) {
                                            AppEngine::instance()->request_redraw_all();
                                        }
                                    });
                                }
                            } else {
                                if (gen_err) g_error_free(gen_err);
                            }

                            {
                                std::lock_guard<std::mutex> lock(s_pending_mutex);
                                s_pending_thumbnails.erase(effective_path);
                            }
                        });
                    }
                }
                // Return null so the view immediately draws a placeholder
                return nullptr;
            }
        }
    }

    int req_w = box_w;
    int req_h = box_h;
    gboolean preserve_aspect = TRUE;

    if (fit_mode == FitMode::Tile) {
        int orig_w = 0, orig_h = 0;
        if (get_image_dimensions(file_to_decode, orig_w, orig_h) && orig_w > 0 && orig_h > 0) {
            req_w = orig_w;
            req_h = orig_h;
        } else {
            req_w = box_w;
            req_h = box_h;
        }
        preserve_aspect = TRUE;
    } else if (fit_mode == FitMode::Center) {
        req_w = (target_size > 0) ? target_size : box_w;
        req_h = (target_size > 0) ? target_size : box_h;
        preserve_aspect = TRUE;
    } else if (fit_mode == FitMode::Fill) {
        req_w = box_w;
        req_h = box_h;
        preserve_aspect = FALSE;
    } else if (fit_mode == FitMode::Cover) {
        int orig_w = 0, orig_h = 0;
        if (get_image_dimensions(file_to_decode, orig_w, orig_h) && orig_w > 0 && orig_h > 0) {
            double scale = std::max(static_cast<double>(box_w) / orig_w, static_cast<double>(box_h) / orig_h);
            req_w = std::max(1, static_cast<int>(std::round(orig_w * scale)));
            req_h = std::max(1, static_cast<int>(std::round(orig_h * scale)));
        } else {
            req_w = std::max(box_w, box_h);
            req_h = req_w;
        }
        preserve_aspect = TRUE;
    } else { // FitMode::Contain
        req_w = box_w;
        req_h = box_h;
        preserve_aspect = TRUE;
    }

    std::string cache_key = file_to_decode + "@" + std::to_string(req_w) + "x" + std::to_string(req_h) +
                            (preserve_aspect ? "p" : "s");
    if (blur_radius > 0) {
        cache_key += "b" + std::to_string(blur_radius);
    }
    if (dim_alpha > 0.001f) {
        cache_key += "d" + std::to_string(static_cast<int>(std::round(dim_alpha * 100)));
    }

    {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_surface_cache.find(cache_key);
        if (it != s_surface_cache.end()) {
            s_surface_lru.splice(s_surface_lru.begin(), s_surface_lru, it->second);
            return it->second->second;
        }
    }

    // Fast disk cache for rendered SVGs / icons (only for clean unblurred icons)
    std::string disk_cache_file = (blur_radius <= 0 && dim_alpha <= 0.001f) ? get_rendered_icon_file(cache_key) : "";
    bool use_disk_cache = false;
    if (!disk_cache_file.empty() && fs::exists(disk_cache_file)) {
        std::error_code dec, oec;
        auto dtime = fs::last_write_time(disk_cache_file, dec);
        auto otime = fs::last_write_time(file_to_decode, oec);
        if (!dec && !oec && dtime >= otime) {
            use_disk_cache = true;
        }
    }

    GError* error = nullptr;
    GdkPixbuf* pixbuf = nullptr;
    if (use_disk_cache) {
        pixbuf = gdk_pixbuf_new_from_file(disk_cache_file.c_str(), &error);
    } else {
        pixbuf = gdk_pixbuf_new_from_file_at_scale(file_to_decode.c_str(), req_w, req_h, preserve_aspect, &error);
        if (pixbuf && !disk_cache_file.empty()) {
            GError* save_err = nullptr;
            gdk_pixbuf_save(pixbuf, disk_cache_file.c_str(), "png", &save_err, "compression", "1", nullptr);
            if (save_err) g_error_free(save_err);
        }
    }

    if (!pixbuf) {
        if (error) g_error_free(error);
        return nullptr;
    }

    cairo_surface_t* surf = pixbuf_to_cairo_surface(pixbuf);
    g_object_unref(pixbuf);

    if (surf && (blur_radius > 0 || dim_alpha > 0.001f)) {
        int sw = cairo_image_surface_get_width(surf);
        int sh = cairo_image_surface_get_height(surf);
        surf = apply_blur_and_dim(surf, sw, sh, blur_radius, dim_alpha);
    }

    if (surf) {
        std::lock_guard<std::mutex> lock(s_cache_mutex);
        auto it = s_surface_cache.find(cache_key);
        if (it != s_surface_cache.end()) {
            cairo_surface_destroy(surf);
            s_surface_lru.splice(s_surface_lru.begin(), s_surface_lru, it->second);
            return it->second->second;
        }

        if (s_surface_cache.size() >= MAX_SURFACE_CACHE_ENTRIES && !s_surface_lru.empty()) {
            auto oldest = s_surface_lru.back();
            s_surface_lru.pop_back();
            s_surface_cache.erase(oldest.first);
            if (oldest.second) {
                cairo_surface_destroy(oldest.second);
            }
        }

        s_surface_lru.push_front({cache_key, surf});
        s_surface_cache[cache_key] = s_surface_lru.begin();
    }
    return surf;
}

void ImageView::preload_surface(const std::string& source, int target_size) {
    if (source.empty() || target_size <= 0) return;
    load_surface(source, target_size, target_size, FitMode::Center, target_size, ImageQuality::FullOriginal);
}

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

ImageView::~ImageView() {
    invalidate_surface_cache();
}

void ImageView::invalidate_surface_cache() {
    if (m_cached_surface) {
        cairo_surface_destroy(m_cached_surface);
        m_cached_surface = nullptr;
    }
    m_cached_w = 0;
    m_cached_h = 0;
    m_cached_source.clear();
    m_cached_fit = FitMode::Contain;
    m_cached_quality = ImageQuality::FullOriginal;
    m_cached_target_size = 0;
    m_cached_blur_radius = 0;
    m_cached_dim_alpha = 0.0f;
}

void ImageView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    int draw_x = bounds.x + m_padding.left;
    int draw_y = bounds.y + m_padding.top;
    int draw_w = std::max(0, bounds.width - m_padding.left - m_padding.right);
    int draw_h = std::max(0, bounds.height - m_padding.top - m_padding.bottom);
    if (draw_w <= 0 || draw_h <= 0) return;

    double cx = draw_x + draw_w / 2.0;
    double cy = draw_y + draw_h / 2.0;
    double radius = std::min(draw_w, draw_h) / 2.0;

    cairo_save(cr);

    // Apply clipping: circle, rounded rect, or rect
    if (m_circle) {
        cairo_arc(cr, cx, cy, radius, 0, 2.0 * M_PI);
        cairo_clip(cr);
    } else if (m_corner_radius > 0) {
        CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, m_corner_radius);
        cairo_clip(cr);
    } else {
        cairo_rectangle(cr, draw_x, draw_y, draw_w, draw_h);
        cairo_clip(cr);
    }

    // Optional rotation around center
    if (std::abs(m_rotation_degrees) > 0.001) {
        double rad = m_rotation_degrees * (M_PI / 180.0);
        cairo_translate(cr, cx, cy);
        cairo_rotate(cr, rad);
        cairo_translate(cr, -cx, -cy);
    }

    // Optional background fill color
    if (m_bg_color.a > 0.0f) {
        cairo_save(cr);
        cairo_set_source_rgba(cr, m_bg_color.r, m_bg_color.g, m_bg_color.b, m_bg_color.a);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    cairo_surface_t* surf = nullptr;
    if (m_cached_surface && m_cached_source == m_source &&
        m_cached_w == draw_w && m_cached_h == draw_h &&
        m_cached_fit == m_fit_mode && m_cached_quality == m_quality &&
        m_cached_target_size == m_target_size &&
        m_cached_blur_radius == m_blur_radius &&
        std::abs(m_cached_dim_alpha - m_dim_alpha) < 0.001f) {
        surf = m_cached_surface;
    } else {
        invalidate_surface_cache();
        cairo_surface_t* loaded = load_surface(m_source, draw_w, draw_h, m_fit_mode, m_target_size, m_quality, m_blur_radius, m_dim_alpha);
        if (loaded) {
            m_cached_surface = cairo_surface_reference(loaded);
            m_cached_w = draw_w;
            m_cached_h = draw_h;
            m_cached_source = m_source;
            m_cached_fit = m_fit_mode;
            m_cached_quality = m_quality;
            m_cached_target_size = m_target_size;
            m_cached_blur_radius = m_blur_radius;
            m_cached_dim_alpha = m_dim_alpha;
            surf = m_cached_surface;
        }
    }

    if (!surf) {
        if (!m_source.empty()) {
            auto config = Config::get();
            // Subtle themed placeholder card
            if (m_circle) {
                cairo_arc(cr, cx, cy, radius, 0, 2.0 * M_PI);
            } else {
                CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, m_corner_radius > 0 ? m_corner_radius : 6.0);
            }
            cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                      config->colors.surface_variant.g,
                                      config->colors.surface_variant.b,
                                      0.45f);
            cairo_fill(cr);

            // Minimalist photo icon outline in center
            int icon_sz = std::min(draw_w, draw_h) / 3;
            if (icon_sz >= 14) {
                double icx = draw_x + (draw_w - icon_sz) / 2.0;
                double icy = draw_y + (draw_h - icon_sz) / 2.0;
                CardView::draw_rounded_rect(cr, icx, icy, icon_sz, icon_sz * 0.75, 3.0);
                cairo_set_source_rgba(cr, config->colors.on_surface_variant.r,
                                          config->colors.on_surface_variant.g,
                                          config->colors.on_surface_variant.b,
                                          0.35f);
                cairo_set_line_width(cr, 1.5);
                cairo_stroke(cr);

                // Small circle inside
                cairo_arc(cr, icx + icon_sz * 0.3, icy + icon_sz * 0.25, icon_sz * 0.1, 0, 2 * M_PI);
                cairo_fill(cr);
            }
        }
        cairo_restore(cr);

        if (m_border_width > 0 && m_border_color.a > 0.0f) {
            cairo_save(cr);
            double stroke_offset = m_border_width / 2.0;
            if (m_circle) {
                cairo_arc(cr, cx, cy, std::max(0.0, radius - stroke_offset), 0, 2.0 * M_PI);
            } else if (m_corner_radius > 0) {
                CardView::draw_rounded_rect(cr, draw_x + stroke_offset,
                                                draw_y + stroke_offset,
                                                draw_w - m_border_width,
                                                draw_h - m_border_width,
                                                std::max(0.0, m_corner_radius - stroke_offset));
            } else {
                cairo_rectangle(cr, draw_x + stroke_offset,
                                    draw_y + stroke_offset,
                                    draw_w - m_border_width,
                                    draw_h - m_border_width);
            }
            cairo_set_source_rgba(cr, m_border_color.r, m_border_color.g, m_border_color.b, m_border_color.a);
            cairo_set_line_width(cr, m_border_width);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
        return;
    }

    if (m_fit_mode == FitMode::Tile) {
        cairo_pattern_t* pat = cairo_pattern_create_for_surface(surf);
        cairo_pattern_set_extend(pat, CAIRO_EXTEND_REPEAT);
        cairo_set_source(cr, pat);
        if (m_opacity < 1.0f) {
            cairo_paint_with_alpha(cr, m_opacity);
        } else {
            cairo_paint(cr);
        }
        cairo_pattern_destroy(pat);
    } else {
        int surf_w = cairo_image_surface_get_width(surf);
        int surf_h = cairo_image_surface_get_height(surf);

        double img_x = draw_x + (draw_w - surf_w) / 2.0;
        double img_y = draw_y + (draw_h - surf_h) / 2.0;

        cairo_set_source_surface(cr, surf, img_x, img_y);
        if (m_opacity < 1.0f) {
            cairo_paint_with_alpha(cr, m_opacity);
        } else {
            cairo_paint(cr);
        }
    }
    cairo_restore(cr);

    // Optional border stroke
    if (m_border_width > 0 && m_border_color.a > 0.0f) {
        cairo_save(cr);
        double stroke_offset = m_border_width / 2.0;
        if (m_circle) {
            cairo_arc(cr, cx, cy, std::max(0.0, radius - stroke_offset), 0, 2.0 * M_PI);
        } else if (m_corner_radius > 0) {
            CardView::draw_rounded_rect(cr, draw_x + stroke_offset,
                                            draw_y + stroke_offset,
                                            draw_w - m_border_width,
                                            draw_h - m_border_width,
                                            std::max(0.0, m_corner_radius - stroke_offset));
        } else {
            cairo_rectangle(cr, draw_x + stroke_offset,
                                draw_y + stroke_offset,
                                draw_w - m_border_width,
                                draw_h - m_border_width);
        }
        cairo_set_source_rgba(cr, m_border_color.r, m_border_color.g, m_border_color.b, m_border_color.a);
        cairo_set_line_width(cr, m_border_width);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

} // namespace miqu
