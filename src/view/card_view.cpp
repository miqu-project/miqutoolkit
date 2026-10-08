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
    int elev = m_elevation;
    if (m_style == CardStyle::Elevated && elev == 0) {
        elev = 2; // Default subtle elevation
    }
    if (elev > 0) {
        cairo_save(cr);
        for (int i = 1; i <= std::min(elev, 4); ++i) {
            double shadow_offset = i * 1.5;
            double shadow_expand = (i - 1) * 0.5;
            double alpha = 0.035 / std::sqrt(static_cast<double>(i));
            draw_rounded_rect(cr, bounds.x - shadow_expand, bounds.y + shadow_offset,
                              bounds.width + shadow_expand * 2.0, bounds.height,
                              radius + shadow_expand);
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, alpha);
            cairo_fill(cr);
        }
        cairo_restore(cr);
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
