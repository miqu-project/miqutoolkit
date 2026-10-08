#pragma once

#include "miqutoolkit/view/view.hpp"
#include "miqutoolkit/core/shm_pool.hpp"
#include "miqutoolkit/core/types.hpp"
#include <wayland-client.h>
#include <xkbcommon/xkbcommon.h>
#include <memory>
#include <functional>
#include <string>

struct zwlr_layer_surface_v1;
struct zwlr_layer_surface_v1_listener;
struct xdg_surface;
struct xdg_surface_listener;
struct xdg_toplevel;
struct xdg_toplevel_listener;
struct wl_cursor_theme;
struct ext_session_lock_surface_v1;
struct ext_session_lock_surface_v1_listener;

namespace miqu {

class AppEngine;
class PopupWindow;

enum class WindowRole {
    Toplevel,        // Standard desktop app window (xdg-shell)
    LayerOverlay,    // Modal overlays
    LayerTop,        // Top bar, panel
    LayerBottom,     // Bottom bar, dock
    LayerBackground, // Wallpaper
    SessionLock,     // ext-session-lock-v1 secure lock surface
};

class Window : public std::enable_shared_from_this<Window> {
public:
    Window();
    ~Window();

    bool init();
    void show();
    void close();
    void request_close();

    void set_content_view(std::shared_ptr<View> view);
    std::shared_ptr<View> get_content_view() const { return m_root_view; }

    void set_role(WindowRole role) { m_role = role; }
    void set_title(std::string title) { m_title = std::move(title); }
    const std::string& get_title() const { return m_title; }
    void set_app_id(std::string app_id) { m_app_id = std::move(app_id); }
    const std::string& get_app_id() const { return m_app_id; }
    void set_layer_namespace(std::string ns) { m_layer_namespace = std::move(ns); }
    const std::string& get_layer_namespace() const { return m_layer_namespace; }

    void set_keyboard_interactive(bool interactive) { m_kb_interactive = interactive; }
    void set_exclusive_zone(int32_t zone) { m_exclusive_zone = zone; }
    void set_preferred_size(int w, int h) { m_width = w; m_height = h; }
    void set_content_size(int w, int h) {
        m_content_w = w;
        m_content_h = h;
        if (m_anchors == 0) {
            m_width = w;
            m_height = h;
        }
    }
    void set_anchors(uint32_t anchors) {
        m_anchors = anchors;
        if (m_anchors == 0 && m_content_w > 0 && m_content_h > 0) {
            m_width = m_content_w;
            m_height = m_content_h;
        }
    }
    void set_output(struct wl_output* output) { m_output = output; }
    struct wl_output* get_output() const { return m_output; }

    // Backdrop & Modal options
    void set_dim_backdrop(bool dim) { m_dim_backdrop = dim; }
    bool is_dim_backdrop() const { return m_dim_backdrop; }
    void set_close_on_click_outside(bool close) { m_close_on_click_outside = close; }
    bool is_close_on_click_outside() const { return m_close_on_click_outside; }
    void set_close_on_escape(bool close) { m_close_on_escape = close; }
    bool is_close_on_escape() const { return m_close_on_escape; }

    // Background & Transparency options
    void set_background_color(const Color& col) { m_background_color = col; m_has_custom_bg = true; }
    const Color& get_background_color() const { return m_background_color; }
    void set_transparent(bool trans) { m_transparent = trans; }
    bool is_transparent() const { return m_transparent; }

    void schedule_redraw();
    void refresh_theme();
    std::shared_ptr<View> root_view() const { return m_root_view; }

    void set_on_close(std::function<void()> cb) { m_on_close = std::move(cb); }
    void set_on_key(std::function<void(const KeyPressEvent&)> cb) { m_on_key = std::move(cb); }

    int get_width() const { return m_width; }
    int get_height() const { return m_height; }
    // Popup overlay support
    void show_popup(std::shared_ptr<PopupWindow> popup);
    void show_popup(std::shared_ptr<View> popup, const Rect& bounds);
    void dismiss_popup();
    bool has_popup() const { return m_popup_view != nullptr || m_active_popup != nullptr; }
    std::shared_ptr<PopupWindow> get_active_popup() const { return m_active_popup; }
    std::shared_ptr<View> get_popup() const { return m_popup_view; }
    const Rect& get_popup_bounds() const { return m_popup_bounds; }

    void update_seat_capabilities(uint32_t caps);

    // Focus Management
    void set_focused_view(View* view);
    void clear_focus();
    View* get_focused_view() const { return m_focused_view; }

private:
    void render_frame();

    static const struct ::zwlr_layer_surface_v1_listener s_layer_surface_listener;
    static const struct ::xdg_surface_listener s_xdg_surface_listener;
    static const struct ::xdg_toplevel_listener s_xdg_toplevel_listener;
    static const struct wl_pointer_listener s_pointer_listener;
    static const struct wl_keyboard_listener s_keyboard_listener;
    static const struct wl_touch_listener s_touch_listener;
    static const struct wl_callback_listener s_frame_listener;

    WindowRole m_role = WindowRole::Toplevel;
    std::string m_title = "miqutoolkit";
    std::string m_app_id = "org.miqu.app";
    std::string m_layer_namespace;

    bool m_kb_interactive = true;
    int32_t m_exclusive_zone = -1;
    uint32_t m_anchors = 0;

    int m_width = 800;
    int m_height = 600;
    int m_content_w = 0;
    int m_content_h = 0;
    bool m_dim_backdrop = false;
    bool m_close_on_click_outside = false;
    bool m_close_on_escape = false;
    Color m_background_color;
    bool m_has_custom_bg = false;
    bool m_transparent = false;

    bool m_configured = false;
    bool m_needs_redraw = false;
    bool m_rendering = false;
    bool m_has_keyboard_focus = false;
    View* m_focused_view = nullptr;
    Rect m_allocated_content_bounds;

    struct wl_surface* m_surface = nullptr;
    struct wl_output* m_output = nullptr;
    struct zwlr_layer_surface_v1* m_layer_surface = nullptr;
    struct ext_session_lock_surface_v1* m_session_lock_surface = nullptr;
    struct xdg_surface* m_xdg_surface = nullptr;
    struct xdg_toplevel* m_xdg_toplevel = nullptr;
    static const struct ::ext_session_lock_surface_v1_listener s_lock_surface_listener;
    struct wl_pointer* m_pointer = nullptr;
    struct wl_keyboard* m_keyboard = nullptr;
    struct wl_touch* m_touch = nullptr;
    int32_t m_primary_touch_id = -1;
    bool m_touch_active = false;
    bool m_touch_consumed_by_view = false;
    struct wl_callback* m_frame_callback = nullptr;
    struct wl_cursor_theme* m_cursor_theme = nullptr;
    struct wl_surface* m_cursor_surface = nullptr;

    void update_cursor(uint32_t serial);

    struct xkb_context* m_xkb_ctx = nullptr;
    struct xkb_keymap* m_xkb_keymap = nullptr;
    struct xkb_state* m_xkb_state = nullptr;
    uint32_t m_modifiers = 0;

    std::unique_ptr<ShmPool> m_shm_pool;
    std::shared_ptr<View> m_root_view;
    std::shared_ptr<PopupWindow> m_active_popup;
    std::shared_ptr<View> m_popup_view;
    Rect m_popup_bounds;

    std::function<void()> m_on_close;
    std::function<void(const KeyPressEvent&)> m_on_key;

    double m_last_x = 0;
    double m_last_y = 0;
};

class WindowBuilder : public std::enable_shared_from_this<WindowBuilder> {
public:
    WindowBuilder() : m_window(std::make_shared<Window>()) {}

    static std::shared_ptr<WindowBuilder> create() {
        return std::make_shared<WindowBuilder>();
    }

    std::shared_ptr<WindowBuilder> role(WindowRole r) {
        m_window->set_role(r);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> title(std::string t) {
        m_window->set_title(std::move(t));
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> appId(std::string id) {
        m_window->set_app_id(std::move(id));
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> layerNamespace(std::string ns) {
        m_window->set_layer_namespace(std::move(ns));
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> keyboardInteractive(bool kb = true) {
        m_window->set_keyboard_interactive(kb);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> exclusiveZone(int32_t zone) {
        m_window->set_exclusive_zone(zone);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> preferredSize(int w, int h) {
        m_window->set_preferred_size(w, h);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> contentSize(int w, int h) {
        m_window->set_content_size(w, h);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> dimBackdrop(bool dim = true) {
        m_window->set_dim_backdrop(dim);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> backgroundColor(const Color& col) {
        m_window->set_background_color(col);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> transparent(bool trans = true) {
        m_window->set_transparent(trans);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> closeOnClickOutside(bool close = true) {
        m_window->set_close_on_click_outside(close);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> closeOnEscape(bool close = true) {
        m_window->set_close_on_escape(close);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> anchors(uint32_t a) {
        m_window->set_anchors(a);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> output(struct wl_output* out) {
        m_window->set_output(out);
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> contentView(std::shared_ptr<View> v) {
        m_window->set_content_view(std::move(v));
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> onClose(std::function<void()> cb) {
        m_window->set_on_close(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<WindowBuilder> onKey(std::function<void(const KeyPressEvent&)> cb) {
        m_window->set_on_key(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<Window> build() {
        if (m_window->init()) {
            return m_window;
        }
        return nullptr;
    }

private:
    std::shared_ptr<Window> m_window;
};

} // namespace miqu
