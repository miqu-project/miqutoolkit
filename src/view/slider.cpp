#include "miqutoolkit/view/slider.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

Size Slider::measure_size() const {
    int w = 120 + m_padding.left + m_padding.right;
    int h = std::max(m_track_height, m_thumb_radius * 2) + 12 + m_padding.top + m_padding.bottom;
    return Size(w, h);
}

void Slider::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();

    int draw_x = bounds.x;
    int draw_y = bounds.y;
    int draw_w = bounds.width;
    int draw_h = bounds.height;

    if (draw_w <= 0 || draw_h <= 0) return;

    Color track_bg = m_custom_track ? m_track_color : config->colors.surface_variant;
    Color prog_color = m_custom_progress ? m_progress_color : config->colors.primary;
    Color thumb_col = m_custom_thumb ? m_thumb_color : config->colors.primary;

    int cy = draw_y + draw_h / 2;
    int th = m_track_height;
    int tr = th / 2;

    int thumb_r = (m_dragging || m_hovered) ? m_thumb_radius + 2 : m_thumb_radius;
    int usable_w = draw_w - thumb_r * 2;
    if (usable_w < 1) usable_w = 1;

    int track_start_x = draw_x + thumb_r;
    int track_width = usable_w;
    int fill_width = static_cast<int>(usable_w * m_value);

    cairo_save(cr);

    // 1. Draw Full Track Background
    CardView::draw_rounded_rect(cr, track_start_x, cy - tr, track_width, th, tr);
    cairo_set_source_rgba(cr, track_bg.r, track_bg.g, track_bg.b, track_bg.a);
    cairo_fill(cr);

    // 2. Draw Active Progress Fill
    if (fill_width > 0) {
        CardView::draw_rounded_rect(cr, track_start_x, cy - tr, fill_width, th, tr);
        cairo_set_source_rgba(cr, prog_color.r, prog_color.g, prog_color.b, prog_color.a);
        cairo_fill(cr);
    }

    // 3. Draw Thumb
    if (thumb_r > 0) {
        int thumb_cx = track_start_x + fill_width;

        // Ambient halo on hover/drag
        if (m_hovered || m_dragging) {
            cairo_arc(cr, thumb_cx, cy, thumb_r + 4, 0, 2 * M_PI);
            cairo_set_source_rgba(cr, thumb_col.r, thumb_col.g, thumb_col.b, 0.25f);
            cairo_fill(cr);
        }

        // Solid thumb circle
        cairo_arc(cr, thumb_cx, cy, thumb_r, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, thumb_col.r, thumb_col.g, thumb_col.b, thumb_col.a);
        cairo_fill(cr);

        // Crisp white inner dot for tactile contrast
        cairo_arc(cr, thumb_cx, cy, std::max(2.0, thumb_r / 2.5), 0, 2 * M_PI);
        cairo_set_source_rgba(cr, 1.0f, 1.0f, 1.0f, 0.9f);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

bool Slider::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    int draw_x = bounds.x;
    int draw_w = bounds.width;
    int thumb_r = m_thumb_radius;
    int usable_w = draw_w - thumb_r * 2;
    if (usable_w < 1) usable_w = 1;

    int track_start_x = draw_x + thumb_r;

    if (pressed) {
        if (!bounds.contains(Point(lx, ly))) {
            return false;
        }
        m_dragging = true;
        float v = std::clamp(static_cast<float>(lx - track_start_x) / usable_w, 0.0f, 1.0f);
        m_value = v;
        if (m_on_change) {
            m_on_change(m_value, true);
        }
        if (m_window) m_window->schedule_redraw();
        return true;
    } else {
        if (m_dragging) {
            m_dragging = false;
            float v = std::clamp(static_cast<float>(lx - track_start_x) / usable_w, 0.0f, 1.0f);
            m_value = v;
            if (m_on_change) {
                m_on_change(m_value, true);
            }
            if (m_window) m_window->schedule_redraw();
            return true;
        }
    }
    return false;
}

bool Slider::on_mouse_move(int lx, int ly, const Rect& bounds) {
    int draw_x = bounds.x;
    int draw_w = bounds.width;
    int thumb_r = m_thumb_radius;
    int usable_w = draw_w - thumb_r * 2;
    if (usable_w < 1) usable_w = 1;

    int track_start_x = draw_x + thumb_r;

    bool was_hovered = m_hovered;
    m_hovered = bounds.contains(Point(lx, ly));

    if (m_dragging) {
        float v = std::clamp(static_cast<float>(lx - track_start_x) / usable_w, 0.0f, 1.0f);
        if (v != m_value) {
            m_value = v;
            if (m_on_change) {
                m_on_change(m_value, true);
            }
            if (m_window) m_window->schedule_redraw();
        }
        return true;
    }

    if (was_hovered != m_hovered && m_window) {
        m_window->schedule_redraw();
    }

    return m_hovered;
}

} // namespace miqu
