#pragma once

#include "miqutoolkit/core/geometry.hpp"
#include "miqutoolkit/core/color.hpp"
#include "miqutoolkit/core/types.hpp"
#include "miqutoolkit/view/layout_params.hpp"
#include <cairo.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <memory>
#include <vector>
#include <functional>

namespace miqu {

class Window;
class PopupMenu;
class ViewGroup;

enum class Visibility {
    Visible,
    Invisible,
    Gone,
};

class View : public std::enable_shared_from_this<View> {
public:
    virtual ~View();

    virtual void draw(cairo_t* cr, const Rect& bounds) = 0;

    virtual Size measure_size() const {
        int w = (m_layout_params.width >= 0) ? m_layout_params.width : m_bounds.width;
        int h = (m_layout_params.height >= 0) ? m_layout_params.height : m_bounds.height;
        return Size(w, h);
    }

    virtual Size measure_size(int avail_width) const {
        (void)avail_width;
        return measure_size();
    }

    virtual bool on_mouse_move(int lx, int ly, const Rect& bounds) { (void)lx; (void)ly; (void)bounds; return false; }
    virtual bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
        if (!is_visible() || !is_enabled() || !bounds.contains(Point(lx, ly))) return false;
        if (button == MouseButton::Right && pressed) {
            return handle_context_click(lx, ly, bounds);
        }
        if (button == MouseButton::Left && pressed) {
            auto self = shared_from_this();
            if (is_focusable()) {
                request_focus();
            }
            request_redraw();
            if (m_on_click) {
                auto cb = m_on_click;
                cb();
                return true;
            }
        }
        return false;
    }
    virtual bool on_key(const KeyPressEvent& event) {
        if (!is_visible() || !is_enabled() || !has_focus()) return false;
        if (event.pressed && (event.keysym == XKB_KEY_space || event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter)) {
            if (m_on_click) {
                auto self = shared_from_this();
                auto cb = m_on_click;
                request_redraw();
                cb();
                return true;
            }
        }
        return false;
    }
    virtual bool on_scroll(double delta) { (void)delta; return false; }
    virtual bool on_touch(const TouchEvent& event, const Rect& bounds) {
        if (!is_visible() || !is_enabled()) return false;
        auto self = shared_from_this();
        int lx = static_cast<int>(event.x);
        int ly = static_cast<int>(event.y);
        switch (event.phase) {
            case TouchPhase::Down:
                on_mouse_move(lx, ly, bounds);
                return on_mouse_button(lx, ly, MouseButton::Left, true, bounds);
            case TouchPhase::Motion:
                return on_mouse_move(lx, ly, bounds);
            case TouchPhase::Up:
            case TouchPhase::Cancel:
                return on_mouse_button(lx, ly, MouseButton::Left, false, bounds);
        }
        return false;
    }

    // Bounds & Geometry
    void set_bounds(const Rect& r) { m_bounds = r; }
    const Rect& get_bounds() const { return m_bounds; }
    virtual bool contains_point(double x, double y) const {
        return m_bounds.contains(static_cast<int>(x), static_cast<int>(y));
    }

    // Padding (Inner spacing)
    void set_padding(const Padding& p) { m_padding = p; }
    void set_padding(int uniform) { m_padding = Padding(uniform); }
    void set_padding(int horizontal, int vertical) { m_padding = Padding(horizontal, vertical); }
    void set_padding(int l, int t, int r, int b) { m_padding = Padding(l, t, r, b); }
    const Padding& get_padding() const { return m_padding; }

    // Margin (Outer spacing)
    void set_margin(const Margin& m) { m_margin = m; }
    void set_margin(int uniform) { m_margin = Margin(uniform); }
    void set_margin(int horizontal, int vertical) { m_margin = Margin(horizontal, vertical); }
    void set_margin(int l, int t, int r, int b) { m_margin = Margin(l, t, r, b); }
    const Margin& get_margin() const { return m_margin; }

    // Layout Params
    void set_layout_params(const LayoutParams& params) { m_layout_params = params; }
    const LayoutParams& get_layout_params() const { return m_layout_params; }
    LayoutParams& get_layout_params() { return m_layout_params; }

    // Content Rectangle after applying padding
    Rect get_content_rect(const Rect& bounds) const {
        int cx = bounds.x + m_padding.left;
        int cy = bounds.y + m_padding.top;
        int cw = std::max(0, bounds.width - m_padding.left - m_padding.right);
        int ch = std::max(0, bounds.height - m_padding.top - m_padding.bottom);
        return Rect(cx, cy, cw, ch);
    }

    // Visibility
    void set_visibility(Visibility v) { m_visibility = v; }
    Visibility get_visibility() const { return m_visibility; }
    bool is_visible() const { return m_visibility == Visibility::Visible; }

    // Enabled State
    void set_enabled(bool enabled) {
        if (m_enabled != enabled) {
            m_enabled = enabled;
            request_redraw();
        }
    }
    bool is_enabled() const { return m_enabled; }

    // Focusability
    void set_focusable(bool focusable) { m_focusable = focusable; }
    bool is_focusable() const { return m_focusable && is_visible() && is_enabled(); }

    // Focus Ring Customization
    virtual void draw_focus_ring(cairo_t* cr, const Rect& bounds);
    void set_draw_focus_ring(bool draw) { m_draw_focus_ring = draw; }
    bool should_draw_focus_ring() const { return m_draw_focus_ring; }
    void set_focus_ring_radius(int radius) { m_focus_ring_radius = radius; }
    int get_focus_ring_radius() const { return m_focus_ring_radius; }

    // Click Listener
    void set_on_click_listener(std::function<void()> listener) {
        m_on_click = std::move(listener);
        if (m_on_click) {
            m_focusable = true;
        }
    }

    // Context Click & Context Menu
    void set_on_context_click_listener(std::function<void(int global_x, int global_y)> listener) {
        m_on_context_click = std::move(listener);
    }
    View* onContextClick(std::function<void(int global_x, int global_y)> listener) {
        m_on_context_click = std::move(listener);
        return this;
    }

    void set_context_menu(std::shared_ptr<PopupMenu> menu);
    View* contextMenu(std::shared_ptr<PopupMenu> menu);

    void set_context_menu_builder(std::function<std::shared_ptr<PopupMenu>(int x, int y)> builder);
    View* contextMenuBuilder(std::function<std::shared_ptr<PopupMenu>(int x, int y)> builder);

    virtual bool handle_context_click(int lx, int ly, const Rect& bounds);

    // Window & Hierarchy Reference
    virtual void set_window(Window* win) { m_window = win; }
    Window* get_window() const { return m_window; }
    void set_parent(ViewGroup* parent) { m_parent = parent; }
    ViewGroup* get_parent() const { return m_parent; }
    void request_redraw();

    // Focus Management
    virtual void on_focus_changed(bool focused) { (void)focused; }
    void request_focus();
    void clear_focus();
    bool has_focus() const;

    // Tooltip Support
    void set_tooltip(std::string text) { m_tooltip = std::move(text); }
    const std::string& get_tooltip() const { return m_tooltip; }
    bool has_tooltip() const { return !m_tooltip.empty(); }

    void set_tooltip_shortcut(std::string shortcut) { m_tooltip_shortcut = std::move(shortcut); }
    const std::string& get_tooltip_shortcut() const { return m_tooltip_shortcut; }
    bool has_tooltip_shortcut() const { return !m_tooltip_shortcut.empty(); }

    virtual std::shared_ptr<View> find_view_at(int lx, int ly, Rect* out_bounds = nullptr);

protected:
    Rect m_bounds;
    Padding m_padding;
    Margin m_margin;
    LayoutParams m_layout_params;
    Visibility m_visibility = Visibility::Visible;
    bool m_enabled = true;
    bool m_focusable = false;
    bool m_draw_focus_ring = true;
    int m_focus_ring_radius = -1;
    Window* m_window = nullptr;
    ViewGroup* m_parent = nullptr;
    std::function<void()> m_on_click;
    std::function<void(int, int)> m_on_context_click;
    std::shared_ptr<PopupMenu> m_context_menu;
    std::function<std::shared_ptr<PopupMenu>(int, int)> m_context_menu_builder;
    std::shared_ptr<PopupMenu> m_active_context_menu;
    std::string m_tooltip;
    std::string m_tooltip_shortcut;
};

} // namespace miqu
