#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

void CardView::draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
    ViewGroup::draw_rounded_rect(cr, x, y, w, h, r);
}

void CardView::draw_elevation_shadow(cairo_t* cr, double x, double y, double w, double h, double radius, double elevation) {
    if (!cr || elevation <= 0.0 || w <= 0.0 || h <= 0.0) return;

    cairo_save(cr);

    // Number of diffusion steps scales smoothly with elevation
    int steps = std::clamp(static_cast<int>(std::ceil(elevation * 3.0)), 2, 14);

    // Total shadow alpha scales smoothly from 0.0 upwards with no artificial hard floor
    double total_alpha = std::clamp(0.12 * std::sqrt(elevation) + 0.05 * elevation, 0.0, 0.45);
    double step_alpha = total_alpha / static_cast<double>(steps);

    double max_spread = elevation * 2.2;
    double max_offset = elevation * 1.0;

    // Use EVEN_ODD fill rule so the shadow ONLY renders outside the bounds,
    // keeping the interior completely untouched and preventing dark color corruption
    cairo_set_fill_rule(cr, CAIRO_FILL_RULE_EVEN_ODD);

    for (int i = 1; i <= steps; ++i) {
        double progress = static_cast<double>(i) / steps;
        double spread = progress * max_spread;
        double offset_y = progress * max_offset;
        double spread_top = spread * 0.5;
        double spread_bottom = spread * 1.5 + offset_y;

        double ox = x - spread;
        double oy = y - spread_top;
        double ow = w + spread * 2.0;
        double oh = h + spread_top + spread_bottom;

        cairo_new_path(cr);
        draw_rounded_rect(cr, ox, oy, ow, oh, radius + spread);
        draw_rounded_rect(cr, x, y, w, h, radius);
        cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, step_alpha);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

void CardView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    int old_r = m_corner_radius;
    Color old_bg = m_background_color;
    bool old_has_bg = m_has_custom_bg;
    double old_sw = m_stroke_width;
    Color old_sc = m_stroke_color;

    int radius = (m_corner_radius < 0) ? config->metrics.corner_radius : m_corner_radius;
    m_corner_radius = radius;

    if (!m_has_custom_bg) {
        m_background_color = config->colors.surface;
        m_has_custom_bg = true;
    }

    // 1. Draw subtle ambient drop shadow if elevated
    double elev = (m_elevation < 0.0) ? config->metrics.card_elevation : m_elevation;
    if (m_style == CardStyle::Elevated && elev <= 0.0) {
        elev = 2.0; // Default subtle elevation
    }
    if (elev > 0.0) {
        draw_elevation_shadow(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius, elev);
    }

    // 2. Configure outline if Outlined style or explicitly stroked
    if (m_style == CardStyle::Outlined && m_stroke_width <= 0) {
        m_stroke_width = 1;
        m_stroke_color = config->colors.outline_variant;
    }

    FrameLayout::draw(cr, bounds);

    m_corner_radius = old_r;
    m_background_color = old_bg;
    m_has_custom_bg = old_has_bg;
    m_stroke_width = old_sw;
    m_stroke_color = old_sc;
}

} // namespace miqu
