#include "miqutoolkit/view/avatar_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <cmath>

namespace miqu {

Size AvatarView::measure_size() const {
    int total_w = m_size + m_padding.left + m_padding.right;
    int total_h = m_size + m_padding.top + m_padding.bottom;
    return Size(total_w, total_h);
}

void AvatarView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    Rect content = get_content_rect(bounds);
    int draw_x = content.x + (content.width - m_size) / 2;
    int draw_y = content.y + (content.height - m_size) / 2;
    int d = m_size;
    if (d <= 0) return;

    auto config = Config::get();

    Color bg;
    if (m_has_custom_bg) {
        bg = m_bg_color;
    } else if (m_selected) {
        bg = config->colors.primary;
    } else {
        bg = config->colors.surface_variant;
    }

    Color border_col;
    if (m_has_custom_border_color) {
        border_col = m_border_color;
    } else if (m_selected) {
        border_col = config->colors.primary;
    } else {
        border_col = config->colors.outline;
    }

    cairo_save(cr);

    // 1. Draw shape background & stroke
    if (m_shape == AvatarShape::Circle) {
        double r = d / 2.0;
        double cx = draw_x + r;
        double cy = draw_y + r;
        cairo_arc(cr, cx, cy, r, 0, 2 * M_PI);
        cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
        cairo_fill_preserve(cr);

        if (m_border_width > 0.0) {
            cairo_set_source_rgba(cr, border_col.r, border_col.g, border_col.b, border_col.a);
            cairo_set_line_width(cr, m_border_width);
            cairo_stroke(cr);
        } else {
            cairo_new_path(cr);
        }
    } else {
        int rad = m_corner_radius > 0 ? m_corner_radius : config->metrics.corner_radius;
        CardView::draw_rounded_rect(cr, draw_x, draw_y, d, d, rad);
        cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
        cairo_fill_preserve(cr);

        if (m_border_width > 0.0) {
            cairo_set_source_rgba(cr, border_col.r, border_col.g, border_col.b, border_col.a);
            cairo_set_line_width(cr, m_border_width);
            cairo_stroke(cr);
        } else {
            cairo_new_path(cr);
        }
    }

    // 2. Content: Image or Text
    if (!m_image_path.empty()) {
        ImageView img(m_image_path);
        img.set_target_size(d - 4);
        img.draw(cr, Rect(draw_x + 2, draw_y + 2, d - 4, d - 4));
    } else if (!m_text.empty()) {
        PangoLayout* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, m_text.c_str(), -1);

        std::string font_family = config->metrics.font_family.empty() ? "Sans" : config->metrics.font_family;
        int fsize = m_font_size > 0 ? m_font_size : std::max(10, d / 3);
        std::string desc_str = font_family + (m_bold ? " Bold " : " ") + std::to_string(fsize);
        PangoFontDescription* desc = pango_font_description_from_string(desc_str.c_str());
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        int tw = 0, th = 0;
        pango_layout_get_pixel_size(layout, &tw, &th);

        double tx = draw_x + (d - tw) / 2.0;
        double ty = draw_y + (d - th) / 2.0;

        Color fg;
        if (m_has_custom_text_color) {
            fg = m_text_color;
        } else if (m_selected) {
            fg = config->colors.on_primary;
        } else {
            fg = config->colors.on_surface;
        }

        cairo_move_to(cr, tx, ty);
        cairo_set_source_rgba(cr, fg.r, fg.g, fg.b, fg.a);
        pango_cairo_show_layout(cr, layout);
        g_object_unref(layout);
    }

    cairo_restore(cr);
}

} // namespace miqu
