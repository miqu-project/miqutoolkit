#pragma once

#include "miqutoolkit/core/color.hpp"
#include <memory>
#include <string>
#include <vector>

#include <functional>

namespace miqu {

class Config {
public:
    struct Colors {
        Color background;
        Color on_background;
        Color surface;
        Color on_surface;
        Color surface_variant;
        Color on_surface_variant;
        Color primary;
        Color on_primary;
        Color primary_container;
        Color on_primary_container;
        Color outline;
        Color outline_variant;
        Color backdrop;
    } colors;

    struct Metrics {
        int corner_radius = 12;
        int border_width = 1;
        std::string icon_theme = "hicolor";
        std::string font_family = "Sans";
        int font_size = 11;
        int h1_size = 20;
        int h2_size = 15;
        int h3_size = 13;
        int caption_size = 10;
    } metrics;

    static std::shared_ptr<Config> get();
    bool load_from_file(const std::string& path);
    void init_toolkit_defaults();
    void set_colors(const Colors& new_colors) { colors = new_colors; }
    void set_metrics(const Metrics& new_metrics) { metrics = new_metrics; }

    const std::vector<std::string>& get_loaded_files() const { return m_loaded_files; }
    void add_change_listener(std::function<void()> listener) { m_change_listeners.push_back(std::move(listener)); }
    void notify_changed();

    static std::string init_user_config(
        const std::string& app_name,
        const std::string& main_file = "",
        const std::vector<std::string>& additional_files = {}
    );

private:
    std::string m_app_config_file;
    std::vector<std::string> m_loaded_files;
    std::vector<std::function<void()>> m_change_listeners;
};

} // namespace miqu

