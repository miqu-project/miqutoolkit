#include "miqutoolkit/view/switch.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

Switch::Switch() = default;

void Switch::set_checked(bool checked) {
    if (m_checked != checked) {
        m_checked = checked;
        if (m_window) m_window->schedule_redraw();
    }
}

Size Switch::measure_size() const {
    int w = (m_layout_params.width >= 0) ? m_layout_params.width : 44;
    int h = (m_layout_params.height >= 0) ? m_layout_params.height : 24;
    return Size(w + m_padding.left + m_padding.right, h + m_padding.top + m_padding.bottom);
}

void Switch::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    Rect content_rect = get_content_rect(bounds);
    if (content_rect.width <= 0 || content_rect.height <= 0) return;

    // Fixed or measured switch dimensions
    double track_w = std::min(static_cast<double>(content_rect.width), 44.0);
    double track_h = std::min(static_cast<double>(content_rect.height), 24.0);

    // Center in content bounds
    double tx = content_rect.x + (content_rect.width - track_w) / 2.0;
    double ty = content_rect.y + (content_rect.height - track_h) / 2.0;
    double radius = track_h / 2.0;

    Color active_track = m_has_custom_track_active ? m_track_active_color : config->colors.primary;
    Color inactive_track = m_has_custom_track_inactive ? m_track_inactive_color : config->colors.surface_variant;
    Color active_thumb = m_has_custom_thumb_active ? m_thumb_active_color : config->colors.on_primary;
    Color inactive_thumb = m_has_custom_thumb_inactive ? m_thumb_inactive_color : Color(1.0f, 1.0f, 1.0f, 1.0f);

    Color current_track = m_checked ? active_track : inactive_track;
    Color current_thumb = m_checked ? active_thumb : inactive_thumb;

    cairo_save(cr);

    // 1. Draw rounded capsule track
    CardView::draw_rounded_rect(cr, tx, ty, track_w, track_h, radius);
    cairo_set_source_rgba(cr, current_track.r, current_track.g, current_track.b, current_track.a);
    cairo_fill(cr);

    if (!m_checked) {
        // Outline stroke for inactive track
        CardView::draw_rounded_rect(cr, tx, ty, track_w, track_h, radius);
        cairo_set_source_rgba(cr, config->colors.outline.r, config->colors.outline.g, config->colors.outline.b, config->colors.outline.a * 0.7);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    // 2. Knob geometry
    double inset = 2.0;
    double knob_d = track_h - (inset * 2.0);
    double knob_r = knob_d / 2.0;
    double knob_cy = ty + track_h / 2.0;
    double knob_cx = m_checked ? (tx + track_w - inset - knob_r) : (tx + inset + knob_r);

    // 3. Ambient hover halo
    if (m_hovered || m_pressed) {
        cairo_arc(cr, knob_cx, knob_cy, knob_r + 4.0, 0, 2 * M_PI);
        if (m_checked) {
            cairo_set_source_rgba(cr, active_track.r, active_track.g, active_track.b, 0.25f);
        } else {
            cairo_set_source_rgba(cr, config->colors.on_surface.r, config->colors.on_surface.g, config->colors.on_surface.b, 0.12f);
        }
        cairo_fill(cr);
    }

    // 4. Subtle drop shadow
    cairo_arc(cr, knob_cx, knob_cy + 1.0, knob_r, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.15);
    cairo_fill(cr);

    // 5. Solid knob
    cairo_arc(cr, knob_cx, knob_cy, knob_r, 0, 2 * M_PI);
    cairo_set_source_rgba(cr, current_thumb.r, current_thumb.g, current_thumb.b, current_thumb.a);
    cairo_fill(cr);

    if (!m_checked) {
        cairo_arc(cr, knob_cx, knob_cy, knob_r, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 0.1);
        cairo_set_line_width(cr, 0.8);
        cairo_stroke(cr);
    }

    cairo_restore(cr);
}

bool Switch::on_mouse_move(int lx, int ly, const Rect& bounds) {
    bool inside = bounds.contains(Point(lx, ly));
    if (inside != m_hovered) {
        m_hovered = inside;
        if (m_window) m_window->schedule_redraw();
    }
    return inside;
}

bool Switch::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    if (pressed) {
        if (!bounds.contains(Point(lx, ly))) return false;
        m_pressed = true;
        if (m_window) m_window->schedule_redraw();
        return true;
    } else {
        if (m_pressed) {
            m_pressed = false;
            if (bounds.contains(Point(lx, ly))) {
                set_checked(!m_checked);
                if (m_on_checked_changed) {
                    m_on_checked_changed(m_checked);
                }
            }
            if (m_window) m_window->schedule_redraw();
            return true;
        }
    }
    return false;
}

SwitchBuilder::SwitchBuilder() : m_view(std::make_shared<Switch>()) {}

} // namespace miqu
