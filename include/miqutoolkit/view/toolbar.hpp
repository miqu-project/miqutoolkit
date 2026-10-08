#pragma once

#include "miqutoolkit/view/view_group.hpp"
#include "miqutoolkit/view/image_button.hpp"
#include "miqutoolkit/view/text_view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <string>
#include <functional>
#include <vector>
#include <memory>

namespace miqu {

namespace icons {
    inline constexpr const char* POWER = "\u23FB";         // ⏻ Power symbol
    inline constexpr const char* REFRESH = "↻";            // ↻ Clockwise reload arrow (\u21BB)
    inline constexpr const char* MENU = "⋮";               // ⋮ Vertical ellipsis (\u22EE)
    inline constexpr const char* MENU_HAMBURGER = "≡";      // ≡ Hamburger menu (\u2261)
    inline constexpr const char* BACK = "‹";               // ‹ Single left-pointing angle (\u2039)
    inline constexpr const char* CLOSE = "✕";              // ✕ Multiplication X (\u2715)
    inline constexpr const char* HOME = "⌂";               // ⌂ House (\u2302)
    inline constexpr const char* SEARCH = "🔍";            // 🔍 Search
    inline constexpr const char* SETTINGS = "⚙";            // ⚙ Gear (\u2699)
}

enum class TitleAlignment {
    Start,   // Left-aligned next to leading icon/navigation
    Center   // Mathematically centered in toolbar width
};

class Toolbar : public ViewGroup {
public:
    Toolbar();
    ~Toolbar() override = default;

    // --- Leading Section ---
    void set_back_button(bool enable, std::function<void()> on_back = nullptr);
    void set_back_visible(bool visible);
    bool is_back_visible() const;

    void set_icon_text(const std::string& icon_text);
    const std::string& get_icon_text() const { return m_icon_text; }

    void set_icon_resource(const std::string& resource_path);
    const std::string& get_icon_resource() const { return m_icon_resource; }

    void set_leading_view(std::shared_ptr<View> view);
    std::shared_ptr<View> get_leading_view() const { return m_leading_custom_view; }

    // --- Title & Center Section ---
    void set_title(const std::string& title);
    const std::string& get_title() const { return m_title; }

    void set_subtitle(const std::string& subtitle);
    const std::string& get_subtitle() const { return m_subtitle; }

    void set_title_alignment(TitleAlignment alignment);
    TitleAlignment get_title_alignment() const { return m_title_alignment; }

    void set_center_view(std::shared_ptr<View> view);
    std::shared_ptr<View> get_center_view() const { return m_center_view; }

    // --- Trailing Actions Section ---
    std::shared_ptr<ImageButton> add_action(const std::string& icon, std::function<void()> on_click);
    void add_action_view(std::shared_ptr<View> view);
    void clear_custom_actions();

    void set_on_menu(std::function<void()> on_menu);
    void set_menu_visible(bool visible);

    void set_on_refresh(std::function<void()> on_refresh);
    void set_refresh_visible(bool visible);

    void set_on_close(std::function<void()> on_close);
    void set_close_visible(bool visible);

    // --- Appearance ---
    void set_show_divider(bool show) { m_show_divider = show; }
    bool get_show_divider() const { return m_show_divider; }

    void set_toolbar_height(int h) { m_toolbar_height = h; }
    int get_toolbar_height() const { return m_toolbar_height; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;
    Size measure_size(int avail_width) const override;

    void set_window(Window* win) override;

private:
    void rebuild_internal_views();

    // Leading components
    bool m_back_enabled = false;
    bool m_back_visible = false;
    std::function<void()> m_on_back;
    std::shared_ptr<ImageButton> m_btn_back;

    std::string m_icon_text;
    std::shared_ptr<TextView> m_icon_view;

    std::string m_icon_resource;
    std::shared_ptr<ImageView> m_icon_img_view;

    std::shared_ptr<View> m_leading_custom_view;

    // Title / Center components
    std::string m_title;
    std::string m_subtitle;
    TitleAlignment m_title_alignment = TitleAlignment::Start;
    std::shared_ptr<TextView> m_title_view;
    std::shared_ptr<TextView> m_subtitle_view;
    std::shared_ptr<View> m_center_view;

    // Trailing components
    std::vector<std::shared_ptr<View>> m_custom_actions;

    bool m_menu_visible = false;
    std::function<void()> m_on_menu;
    std::shared_ptr<ImageButton> m_btn_menu;

    bool m_refresh_visible = false;
    std::function<void()> m_on_refresh;
    std::shared_ptr<ImageButton> m_btn_refresh;

    bool m_close_visible = false;
    std::function<void()> m_on_close;
    std::shared_ptr<ImageButton> m_btn_close;

    // Options
    bool m_show_divider = false;
    int m_toolbar_height = 48;
};

class ToolbarBuilder : public std::enable_shared_from_this<ToolbarBuilder> {
public:
    ToolbarBuilder() : m_toolbar(std::make_shared<Toolbar>()) {}

    static std::shared_ptr<ToolbarBuilder> create() {
        return std::make_shared<ToolbarBuilder>();
    }

    std::shared_ptr<ToolbarBuilder> title(const std::string& title) {
        m_toolbar->set_title(title);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> subtitle(const std::string& subtitle) {
        m_toolbar->set_subtitle(subtitle);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> titleAlignment(TitleAlignment align) {
        m_toolbar->set_title_alignment(align);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> iconText(const std::string& icon) {
        m_toolbar->set_icon_text(icon);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> iconResource(const std::string& res) {
        m_toolbar->set_icon_resource(res);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> backButton(bool enable, std::function<void()> on_back = nullptr) {
        m_toolbar->set_back_button(enable, std::move(on_back));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> onBack(std::function<void()> on_back) {
        m_toolbar->set_back_button(true, std::move(on_back));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> leadingView(std::shared_ptr<View> view) {
        m_toolbar->set_leading_view(std::move(view));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> centerView(std::shared_ptr<View> view) {
        m_toolbar->set_center_view(std::move(view));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> onRefresh(std::function<void()> on_refresh) {
        m_toolbar->set_on_refresh(std::move(on_refresh));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> onClose(std::function<void()> on_close) {
        m_toolbar->set_on_close(std::move(on_close));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> onMenu(std::function<void()> on_menu) {
        m_toolbar->set_on_menu(std::move(on_menu));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> addAction(const std::string& icon, std::function<void()> on_click) {
        m_toolbar->add_action(icon, std::move(on_click));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> addActionView(std::shared_ptr<View> view) {
        m_toolbar->add_action_view(std::move(view));
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> showDivider(bool show) {
        m_toolbar->set_show_divider(show);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> toolbarHeight(int h) {
        m_toolbar->set_toolbar_height(h);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> padding(int horizontal, int vertical) {
        m_toolbar->set_padding(horizontal, vertical);
        return shared_from_this();
    }

    std::shared_ptr<ToolbarBuilder> padding(int left, int top, int right, int bottom) {
        m_toolbar->set_padding(left, top, right, bottom);
        return shared_from_this();
    }

    std::shared_ptr<Toolbar> build() {
        return m_toolbar;
    }

private:
    std::shared_ptr<Toolbar> m_toolbar;
};

} // namespace miqu
