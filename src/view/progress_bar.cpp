#include "miqutoolkit/view/progress_bar.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/core/app_engine.hpp"
#include <cmath>
#include <chrono>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

void ProgressBar::set_indeterminate(bool indeterminate) {
    m_indeterminate = indeterminate;
    if (m_indeterminate) {
        if (m_window) {
            m_window->schedule_redraw();
        } else if (auto* engine = AppEngine::instance()) {
            engine->request_redraw_all();
        }
    }
}

Size ProgressBar::measure_size() const {
    if (m_style == ProgressBarStyle::Circular) {
        int def = 36;
        int w = (m_layout_params.width >= 0) ? m_layout_params.width : def;
        int h = (m_layout_params.height >= 0) ? m_layout_params.height : def;
        return Size(w + m_padding.left + m_padding.right, h + m_padding.top + m_padding.bottom);
    } else {
        int h = m_track_height + m_padding.top + m_padding.bottom;
        int w = (m_layout_params.width >= 0) ? m_layout_params.width : (100 + m_padding.left + m_padding.right);
        return Size(w, h);
    }
}

void ProgressBar::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    Color track_col = m_has_custom_track ? m_track_color : config->colors.surface_variant;
    Color prog_col = m_has_custom_progress ? m_progress_color : config->colors.primary;

    Rect content_rect = get_content_rect(bounds);
    if (content_rect.width <= 0 || content_rect.height <= 0) return;

    if (m_style == ProgressBarStyle::Circular) {
        double diameter = std::min(content_rect.width, content_rect.height);
        double stroke_w = std::min(static_cast<double>(m_track_height), diameter / 2.0);
        double radius = (diameter - stroke_w) / 2.0;

        if (radius <= 0.0) return;

        double cx = content_rect.x + content_rect.width / 2.0;
        double cy = content_rect.y + content_rect.height / 2.0;

        cairo_save(cr);
        cairo_new_path(cr);
        cairo_set_line_width(cr, stroke_w);

        // 1. Draw circular background track
        cairo_new_sub_path(cr);
        cairo_set_source_rgba(cr, track_col.r, track_col.g, track_col.b, track_col.a);
        cairo_arc(cr, cx, cy, radius, 0.0, 2.0 * M_PI);
        cairo_stroke(cr);

        // 2. Draw circular active progress arc
        if (m_indeterminate) {
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            double rot = std::fmod(static_cast<double>(now_ms), 1000.0) / 1000.0 * (2.0 * M_PI);
            double pulse = (std::sin(static_cast<double>(now_ms) / 1500.0 * (2.0 * M_PI)) * 0.5 + 0.5);
            double sweep = (0.35 + pulse * 1.15) * M_PI;

            cairo_new_sub_path(cr);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_source_rgba(cr, prog_col.r, prog_col.g, prog_col.b, prog_col.a);
            double start_angle = -M_PI / 2.0 + rot;
            double end_angle = start_angle + sweep;
            cairo_arc(cr, cx, cy, radius, start_angle, end_angle);
            cairo_stroke(cr);
        } else if (m_progress > 0.0f) {
            cairo_new_sub_path(cr);
            cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);
            cairo_set_source_rgba(cr, prog_col.r, prog_col.g, prog_col.b, prog_col.a);
            double start_angle = -M_PI / 2.0; // 12 o'clock
            double end_angle = start_angle + m_progress * (2.0 * M_PI);
            cairo_arc(cr, cx, cy, radius, start_angle, end_angle);
            cairo_stroke(cr);
        }

        cairo_new_path(cr);
        cairo_restore(cr);
    } else {
        // Linear Progress Bar
        double h = std::min(content_rect.height, m_track_height);
        double y = content_rect.y + (content_rect.height - h) / 2.0;
        double x = content_rect.x;
        double w = content_rect.width;
        double radius = (m_corner_radius >= 0) ? m_corner_radius : (h / 2.0);

        cairo_save(cr);

        // 1. Draw rounded background track
        CardView::draw_rounded_rect(cr, x, y, w, h, radius);
        cairo_set_source_rgba(cr, track_col.r, track_col.g, track_col.b, track_col.a);
        cairo_fill(cr);

        // 2. Draw progress fill as a rounded capsule clipped to track pill
        if (m_indeterminate) {
            auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now().time_since_epoch()).count();
            double cycle = std::fmod(static_cast<double>(now_ms), 1400.0) / 1400.0;
            CardView::draw_rounded_rect(cr, x, y, w, h, radius);
            cairo_clip(cr);

            double pill_w = std::max(h * 2.0, w * 0.35);
            double pill_x = x - pill_w + cycle * (w + pill_w);
            CardView::draw_rounded_rect(cr, pill_x, y, pill_w, h, radius);
            cairo_set_source_rgba(cr, prog_col.r, prog_col.g, prog_col.b, prog_col.a);
            cairo_fill(cr);
        } else if (m_progress > 0.0f) {
            CardView::draw_rounded_rect(cr, x, y, w, h, radius);
            cairo_clip(cr);

            double fill_w = std::max(static_cast<double>(h), w * m_progress);
            CardView::draw_rounded_rect(cr, x, y, fill_w, h, radius);
            cairo_set_source_rgba(cr, prog_col.r, prog_col.g, prog_col.b, prog_col.a);
            cairo_fill(cr);
        }

        cairo_restore(cr);
    }

    // Schedule next frame animation if indeterminate and visible
    if (m_indeterminate && is_visible()) {
        if (m_window) {
            m_window->schedule_redraw();
        } else if (auto* engine = AppEngine::instance()) {
            engine->request_redraw_all();
        }
    }
}

} // namespace miqu
