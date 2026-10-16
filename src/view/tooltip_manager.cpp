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
    if (!window || !window->get_content_view()) return;

    Rect bounds;
    auto view = window->get_content_view()->find_view_at(static_cast<int>(x), static_cast<int>(y), &bounds);
    std::string tooltip_text = (view && view->has_tooltip()) ? view->get_tooltip() : "";

    if (tooltip_text.empty()) {
        if (m_is_showing || m_show_timer_id != 0) {
            dismiss();
        }
        return;
    }

    auto current_view = m_hovered_view.lock();
    if (m_active_window == window && current_view == view && m_pending_text == tooltip_text) {
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
        m_inwin_tooltip_view = nullptr;
        m_is_showing = false;
        if (m_overlay_window) {
            m_overlay_window->close();
            m_overlay_window = nullptr;
        }
    }
}

void TooltipManager::show_tooltip(Window* window, const std::string& text, const Rect& anchor_bounds) {
    if (!window || text.empty()) return;

    dismiss();
    m_active_window = window;
    m_anchor_bounds = anchor_bounds;
    m_pending_text = text;
    trigger_show();
}

void TooltipManager::trigger_show() {
    if (!m_active_window || m_pending_text.empty()) return;

    WindowRole role = m_active_window->get_role();
    if (role == WindowRole::LayerTop || role == WindowRole::LayerBottom || role == WindowRole::LayerOverlay || role == WindowRole::LayerBackground) {
        show_for_layer_window(m_active_window, m_pending_text, m_anchor_bounds);
    } else {
        show_for_toplevel_window(m_active_window, m_pending_text, m_anchor_bounds);
    }

    auto* engine = AppEngine::instance();
    if (engine && m_auto_dismiss_ms > 0) {
        m_dismiss_timer_id = engine->post_delayed(m_auto_dismiss_ms, [this]() {
            m_dismiss_timer_id = 0;
            dismiss();
        });
    }
}

void TooltipManager::show_for_layer_window(Window* window, const std::string& text, const Rect& anchor_bounds) {
    if (m_overlay_window) {
        m_overlay_window->close();
        m_overlay_window = nullptr;
    }

    if (!m_overlay_tooltip_view) {
        m_overlay_tooltip_view = std::make_shared<TooltipView>();
    }
    m_overlay_tooltip_view->set_text(text);

    Size size = m_overlay_tooltip_view->measure_size();
    int tip_w = size.width;
    int tip_h = size.height;

    int win_w = window->get_width() > 0 ? window->get_width() : 1920;
    int anchor_w = anchor_bounds.width > 0 ? anchor_bounds.width : 32;
    int target_x = anchor_bounds.x + (anchor_w - tip_w) / 2;
    if (target_x < 8) target_x = 8;
    if (target_x + tip_w > win_w - 8) {
        target_x = std::max(8, win_w - tip_w - 8);
    }

    bool is_bottom = (window->get_role() == WindowRole::LayerBottom) ||
                     ((window->get_anchors() & ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM) != 0);

    uint32_t anchors = ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT;
    Margin margin;
    margin.left = target_x;

    if (is_bottom) {
        anchors |= ZWLR_LAYER_SURFACE_V1_ANCHOR_BOTTOM;
        margin.bottom = window->get_height() + 6;
    } else {
        anchors |= ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP;
        margin.top = window->get_height() + 6;
    }

    m_overlay_window = std::make_shared<Window>();
    m_overlay_window->set_role(WindowRole::LayerOverlay);
    m_overlay_window->set_output(window->get_output());
    m_overlay_window->set_layer_namespace("miqu-tooltip");
    m_overlay_window->set_anchors(anchors);
    m_overlay_window->set_exclusive_zone(-1);
    m_overlay_window->set_content_margin(margin);
    m_overlay_window->set_preferred_size(tip_w, tip_h);
    m_overlay_window->set_content_size(tip_w, tip_h);
    m_overlay_window->set_keyboard_interactive(false);
    m_overlay_window->set_transparent(true);
    m_overlay_window->set_content_view(m_overlay_tooltip_view);

    if (!m_overlay_window->init()) {
        m_overlay_window = nullptr;
        return;
    }
    m_overlay_window->show();
    m_is_showing = true;
}

void TooltipManager::show_for_toplevel_window(Window* window, const std::string& text, const Rect& anchor_bounds) {
    if (!m_inwin_tooltip_view) {
        m_inwin_tooltip_view = std::make_shared<TooltipView>();
    }
    m_inwin_tooltip_view->set_text(text);

    Size size = m_inwin_tooltip_view->measure_size();
    int tip_w = size.width;
    int tip_h = size.height;

    int win_w = window->get_width() > 0 ? window->get_width() : 800;
    int win_h = window->get_height() > 0 ? window->get_height() : 600;

    int target_x = anchor_bounds.x + (anchor_bounds.width - tip_w) / 2;
    if (target_x < 8) target_x = 8;
    if (target_x + tip_w > win_w - 8) {
        target_x = std::max(8, win_w - tip_w - 8);
    }

    int target_y = anchor_bounds.y + anchor_bounds.height + 6;
    if (target_y + tip_h > win_h - 8) {
        target_y = std::max(8, anchor_bounds.y - tip_h - 6);
    }
    if (target_y < 8) target_y = 8;

    window->show_tooltip_overlay(m_inwin_tooltip_view, Rect(target_x, target_y, tip_w, tip_h));
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
}

} // namespace miqu
