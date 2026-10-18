#include "miqutoolkit/view/tooltip_manager.hpp"
#include "miqutoolkit/view/tooltip_view.hpp"
#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include <algorithm>

namespace miqu {

TooltipManager* TooltipManager::s_instance = nullptr;

TooltipManager* TooltipManager::get() {
    if (!s_instance) {
        s_instance = new TooltipManager();
    }
    return s_instance;
}

TooltipManager::~TooltipManager() {
    dismiss();
    if (s_instance == this) {
        s_instance = nullptr;
    }
}

void TooltipManager::on_pointer_motion(Window* window, double x, double y) {
    if (!window) return;

    std::shared_ptr<View> view = nullptr;
    Rect bounds;

    // 1. If an active popup overlay is visible and contains pointer, search inside popup first
    if (window->has_popup() && window->get_popup()) {
        const Rect& popup_bounds = window->get_popup_bounds();
        if (popup_bounds.contains(Point(static_cast<int>(x), static_cast<int>(y)))) {
            view = window->get_popup()->find_view_at(static_cast<int>(x), static_cast<int>(y), &bounds);
        }
    }

    // 2. Otherwise search inside main content tree
    if (!view && window->get_content_view()) {
        view = window->get_content_view()->find_view_at(static_cast<int>(x), static_cast<int>(y), &bounds);
    }

    std::string tooltip_text = (view && view->has_tooltip()) ? view->get_tooltip() : "";
    std::string tooltip_shortcut = (view && view->has_tooltip_shortcut()) ? view->get_tooltip_shortcut() : "";

    if (tooltip_text.empty()) {
        if (m_is_showing || m_show_timer_id != 0) {
            dismiss();
        }
        return;
    }

    auto current_view = m_hovered_view.lock();
    if (m_active_window == window && current_view == view &&
        m_pending_text == tooltip_text && m_pending_shortcut == tooltip_shortcut) {
        return;
    }

    auto* engine = AppEngine::instance();
    if (engine) {
        if (m_show_timer_id != 0) {
            engine->cancel_delayed(m_show_timer_id);
            m_show_timer_id = 0;
        }
        if (m_dismiss_timer_id != 0) {
            engine->cancel_delayed(m_dismiss_timer_id);
            m_dismiss_timer_id = 0;
        }
    }

    m_active_window = window;
    m_hovered_view = view;
    m_anchor_bounds = bounds;
    m_pending_text = tooltip_text;
    m_pending_shortcut = tooltip_shortcut;

    // Fast-path switch (50ms) if already showing another tooltip
    int delay = m_is_showing ? 50 : m_show_delay_ms;
    if (engine) {
        m_show_timer_id = engine->post_delayed(delay, [this]() {
            m_show_timer_id = 0;
            trigger_show();
        });
    }
}

void TooltipManager::on_pointer_leave(Window* window) {
    if (m_active_window == window) {
        dismiss();
    }
}

void TooltipManager::on_pointer_button(Window* window) {
    (void)window;
    dismiss();
}

void TooltipManager::on_window_destroyed(Window* window) {
    if (m_overlay_window && m_overlay_window.get() == window) {
        m_overlay_window = nullptr;
    }
    if (m_active_window == window) {
        auto* engine = AppEngine::instance();
        if (engine) {
            if (m_show_timer_id != 0) engine->cancel_delayed(m_show_timer_id);
            if (m_dismiss_timer_id != 0) engine->cancel_delayed(m_dismiss_timer_id);
        }
        m_show_timer_id = 0;
        m_dismiss_timer_id = 0;
        m_active_window = nullptr;
        m_hovered_view.reset();
        m_pending_text.clear();
        m_pending_shortcut.clear();
        m_inwin_tooltip_view = nullptr;
        m_is_showing = false;
        if (m_overlay_window) {
            m_overlay_window->close();
            m_overlay_window = nullptr;
        }
    }
}

void TooltipManager::show_tooltip(Window* window, const std::string& text, const Rect& anchor_bounds) {
    show_tooltip(window, text, "", anchor_bounds);
}

void TooltipManager::show_tooltip(Window* window, const std::string& text, const std::string& shortcut, const Rect& anchor_bounds) {
    if (!window || text.empty()) return;

    dismiss();
    m_active_window = window;
    m_anchor_bounds = anchor_bounds;
    m_pending_text = text;
    m_pending_shortcut = shortcut;
    trigger_show();
}

bool TooltipManager::should_use_in_window_overlay(Window* window) const {
    if (!window) return false;

    // Toplevel windows render tooltips in-window
    WindowRole role = window->get_role();
    if (role == WindowRole::Toplevel) {
        return true;
    }

    // For layer windows:
    // If the window has ample dimensions in both axes (e.g. drawers like miqunotify-center,
    // launchers like miqulauncher, or desktop views), in-window overlay is much faster,
    // perfectly synchronized in the same frame buffer, and has zero coordinate ambiguity.
    int w = window->get_width();
    int h = window->get_height();
    if (w > 64 && h > 64) {
        return true;
    }

    // Thin edge bars (e.g. miqubar: height or width <= 64) cannot fit tooltips internally
    // without clipping by surface bounds, so they require an external layer surface.
    return false;
}

void TooltipManager::trigger_show() {
    if (!m_active_window || m_pending_text.empty()) return;

    if (should_use_in_window_overlay(m_active_window)) {
        show_in_window(m_active_window, m_pending_text, m_pending_shortcut, m_anchor_bounds);
    } else {
        show_for_layer_window(m_active_window, m_pending_text, m_pending_shortcut, m_anchor_bounds);
    }

    auto* engine = AppEngine::instance();
    if (engine && m_auto_dismiss_ms > 0) {
        m_dismiss_timer_id = engine->post_delayed(m_auto_dismiss_ms, [this]() {
            m_dismiss_timer_id = 0;
            dismiss();
        });
    }
}

void TooltipManager::show_in_window(Window* window, const std::string& text, const std::string& shortcut, const Rect& anchor_bounds) {
    if (!window) return;
    if (!m_inwin_tooltip_view) {
        m_inwin_tooltip_view = std::make_shared<TooltipView>();
    }
    m_inwin_tooltip_view->set_text(text);
    m_inwin_tooltip_view->set_shortcut(shortcut);

    Size size = m_inwin_tooltip_view->measure_size();
    int tip_w = size.width;
    int tip_h = size.height;

    int win_w = window->get_width() > 0 ? window->get_width() : 800;
    int win_h = window->get_height() > 0 ? window->get_height() : 600;

    // Center horizontally relative to anchor view, clamped to window padding
    int target_x = anchor_bounds.x + (anchor_bounds.width - tip_w) / 2;
    if (target_x < 8) target_x = 8;
    if (target_x + tip_w > win_w - 8) {
        target_x = std::max(8, win_w - tip_w - 8);
    }

    // Vertical placement: prefer below anchor if space permits; otherwise flip above
    int target_y = 0;
    if (anchor_bounds.y + anchor_bounds.height + 6 + tip_h <= win_h - 8) {
        target_y = anchor_bounds.y + anchor_bounds.height + 6;
    } else if (anchor_bounds.y - tip_h - 6 >= 8) {
        target_y = anchor_bounds.y - tip_h - 6;
    } else {
        target_y = std::max(8, anchor_bounds.y - tip_h - 6);
        if (target_y + tip_h > win_h - 8) {
            target_y = std::max(8, win_h - tip_h - 8);
        }
    }

    window->show_tooltip_overlay(m_inwin_tooltip_view, Rect(target_x, target_y, tip_w, tip_h));
    m_is_showing = true;
}

void TooltipManager::show_for_toplevel_window(Window* window, const std::string& text, const Rect& anchor_bounds) {
    show_in_window(window, text, "", anchor_bounds);
}

void TooltipManager::show_for_layer_window(Window* window, const std::string& text, const std::string& shortcut, const Rect& anchor_bounds) {
    if (m_overlay_window) {
        m_overlay_window->close();
        m_overlay_window = nullptr;
    }

    if (!m_overlay_tooltip_view) {
        m_overlay_tooltip_view = std::make_shared<TooltipView>();
    }
    m_overlay_tooltip_view->set_text(text);
    m_overlay_tooltip_view->set_shortcut(shortcut);

    Size size = m_overlay_tooltip_view->measure_size();
    int tip_w = size.width;
    int tip_h = size.height;

    int win_w = window->get_width() > 0 ? window->get_width() : 1920;
    int win_h = window->get_height() > 0 ? window->get_height() : 32;

    uint32_t anchors = 0;
    Margin margin;

    // Check whether bar is horizontal or vertical
    if (win_h <= 64) {
        // Horizontal bar (top or bottom)
        int anchor_w = anchor_bounds.width > 0 ? anchor_bounds.width : 32;
        int target_x = anchor_bounds.x + (anchor_w - tip_w) / 2;
        if (target_x < 8) target_x = 8;
        if (target_x + tip_w > win_w - 8) {
            target_x = std::max(8, win_w - tip_w - 8);
        }

        bool is_bottom = (window->get_role() == WindowRole::LayerBottom) ||
                         ((window->get_anchors() & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) != 0);

        anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
        margin.left = target_x;

        if (is_bottom) {
            anchors |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
            margin.bottom = win_h + 6;
        } else {
            anchors |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
            margin.top = win_h + 6;
        }
    } else {
        // Vertical bar (left or right)
        bool is_right = (window->get_anchors() & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT) != 0;
        int target_y = anchor_bounds.y + (anchor_bounds.height - tip_h) / 2;
        if (target_y < 8) target_y = 8;

        anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        margin.top = target_y;

        if (is_right) {
            anchors |= ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT;
            margin.right = win_w + 6;
        } else {
            anchors |= ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
            margin.left = win_w + 6;
        }
    }

    auto overlay = std::make_shared<Window>();
    overlay->set_role(WindowRole::LayerOverlay);
    overlay->set_output(window->get_output());
    overlay->set_layer_namespace("miqu-tooltip");
    overlay->set_anchors(anchors);
    overlay->set_exclusive_zone(-1);
    overlay->set_content_margin(margin);
    overlay->set_preferred_size(tip_w, tip_h);
    overlay->set_content_size(tip_w, tip_h);
    overlay->set_keyboard_interactive(false);
    overlay->set_transparent(true);
    overlay->set_content_view(m_overlay_tooltip_view);

    m_overlay_window = overlay;

    if (!overlay->init()) {
        if (m_overlay_window == overlay) {
            m_overlay_window = nullptr;
        }
        return;
    }

    // Guard against dismissal during init()'s synchronous wl_display_roundtrip
    if (m_overlay_window != overlay || !m_overlay_window) {
        overlay->close();
        return;
    }

    m_overlay_window->show();
    m_is_showing = true;
}

void TooltipManager::dismiss() {
    auto* engine = AppEngine::instance();
    if (engine) {
        if (m_show_timer_id != 0) {
            engine->cancel_delayed(m_show_timer_id);
            m_show_timer_id = 0;
        }
        if (m_dismiss_timer_id != 0) {
            engine->cancel_delayed(m_dismiss_timer_id);
            m_dismiss_timer_id = 0;
        }
    }

    if (m_overlay_window) {
        m_overlay_window->close();
        m_overlay_window = nullptr;
    }

    if (m_inwin_tooltip_view && m_active_window) {
        m_active_window->dismiss_tooltip_overlay();
        m_inwin_tooltip_view = nullptr;
    }

    m_is_showing = false;
    m_hovered_view.reset();
    m_pending_text.clear();
    m_pending_shortcut.clear();
}

} // namespace miqu
