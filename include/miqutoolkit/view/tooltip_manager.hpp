#pragma once

#include "miqutoolkit/view/view.hpp"
#include <memory>
#include <string>

namespace miqu {

class Window;
class TooltipView;

/**
 * @brief Coordinates tooltip hover detection, delayed appearance, positioning,
 * and dismissal across all windows in the application.
 */
class TooltipManager {
public:
    static TooltipManager* get();
    ~TooltipManager();

    // Event hooks invoked automatically by Window
    void on_pointer_motion(Window* window, double x, double y);
    void on_pointer_leave(Window* window);
    void on_pointer_button(Window* window);
    void on_window_destroyed(Window* window);

    // Explicit presentation control
    void show_tooltip(Window* window, const std::string& text, const Rect& anchor_bounds);
    void dismiss();

    // Tunables
    void set_show_delay(int ms) { m_show_delay_ms = ms; }
    int get_show_delay() const { return m_show_delay_ms; }

    void set_auto_dismiss_timeout(int ms) { m_auto_dismiss_ms = ms; }
    int get_auto_dismiss_timeout() const { return m_auto_dismiss_ms; }

    bool is_showing() const { return m_is_showing; }

private:
    TooltipManager() = default;

    void trigger_show();
    void show_for_layer_window(Window* window, const std::string& text, const Rect& anchor_bounds);
    void show_for_toplevel_window(Window* window, const std::string& text, const Rect& anchor_bounds);

    static TooltipManager* s_instance;

    int m_show_delay_ms = 400;
    int m_auto_dismiss_ms = 5000;

    uint64_t m_show_timer_id = 0;
    uint64_t m_dismiss_timer_id = 0;

    Window* m_active_window = nullptr;
    std::weak_ptr<View> m_hovered_view;
    Rect m_anchor_bounds;
    std::string m_pending_text;

    std::shared_ptr<Window> m_overlay_window;
    std::shared_ptr<TooltipView> m_overlay_tooltip_view;
    std::shared_ptr<TooltipView> m_inwin_tooltip_view;

    bool m_is_showing = false;
};

} // namespace miqu
