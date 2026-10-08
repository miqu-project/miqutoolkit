#include "miqutoolkit/view/divider_view.hpp"
#include "miqutoolkit/core/config.hpp"

namespace miqu {

Size DividerView::measure_size() const {
    if (m_orientation == Orientation::Horizontal) {
        return Size(0, m_thickness + m_padding.top + m_padding.bottom);
    } else {
        return Size(m_thickness + m_padding.left + m_padding.right, 0);
    }
}

void DividerView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    Rect content_bounds = get_content_rect(bounds);
    if (content_bounds.width <= 0 || content_bounds.height <= 0) return;

    auto config = Config::get();
    Color div_color = m_has_custom_color ? m_color : config->colors.outline_variant;
    if (div_color.a <= 0.0f) return;

    cairo_save(cr);
    cairo_set_source_rgba(cr, div_color.r, div_color.g, div_color.b, div_color.a);

    if (m_orientation == Orientation::Horizontal) {
        int line_y = content_bounds.y + (content_bounds.height - m_thickness) / 2;
        cairo_rectangle(cr, content_bounds.x, line_y, content_bounds.width, m_thickness);
    } else {
        int line_x = content_bounds.x + (content_bounds.width - m_thickness) / 2;
        cairo_rectangle(cr, line_x, content_bounds.y, m_thickness, content_bounds.height);
    }

    cairo_fill(cr);
    cairo_restore(cr);
}

} // namespace miqu
