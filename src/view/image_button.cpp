#include "miqutoolkit/view/image_button.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

Size ImageButton::measure_size() const {
    int pad_h = m_padding.left + m_padding.right > 0 ? (m_padding.left + m_padding.right) : 16;
    int pad_v = m_padding.top + m_padding.bottom > 0 ? (m_padding.top + m_padding.bottom) : 16;
    int s = std::max(m_icon_size + pad_h, m_icon_size + pad_v);
    return Size(s, s);
}

void ImageButton::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_bounds = bounds;
    auto config = Config::get();

    int draw_x = bounds.x;
    int draw_y = bounds.y;
    int draw_w = bounds.width;
    int draw_h = bounds.height;

    if (draw_w <= 0 || draw_h <= 0) return;

    Color bg_col = m_custom_bg ? m_bg_color : Color::transparent();
    if (m_pressed) {
        bg_col = config->colors.primary.with_alpha(0.3f);
    } else if (m_hovered) {
        bg_col = m_custom_hover ? m_hover_color : config->colors.surface_variant.with_alpha(0.5f);
    }

    cairo_save(cr);

    // Draw Background
    if (bg_col.a > 0.0f) {
        if (m_circle) {
            double cx = draw_x + draw_w / 2.0;
            double cy = draw_y + draw_h / 2.0;
            double r = std::min(draw_w, draw_h) / 2.0;
            cairo_arc(cr, cx, cy, r, 0, 2.0 * M_PI);
            cairo_set_source_rgba(cr, bg_col.r, bg_col.g, bg_col.b, bg_col.a);
            cairo_fill(cr);
        } else {
            int radius = (m_corner_radius >= 0) ? m_corner_radius : std::max(0, config->metrics.corner_radius);
            CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, radius);
            cairo_set_source_rgba(cr, bg_col.r, bg_col.g, bg_col.b, bg_col.a);
            cairo_fill(cr);
        }
    }

    // Draw Image resource or Font Icon
    if (!m_image_resource.empty()) {
        int icon_s = m_icon_size > 0 ? m_icon_size : 20;
        int ix = draw_x + (draw_w - icon_s) / 2;
        int iy = draw_y + (draw_h - icon_s) / 2;

        ImageView temp_img(m_image_resource);
        temp_img.set_target_size(icon_s);
        temp_img.draw(cr, Rect(ix, iy, icon_s, icon_s));
    } else if (!m_icon.empty()) {
        std::string res_path = ImageView::resolve_icon_path(m_icon);
        bool is_img_icon = !res_path.empty() || m_icon.starts_with('/') || m_icon.ends_with(".png") || m_icon.ends_with(".svg");
        if (is_img_icon) {
            int icon_s = m_icon_size > 0 ? m_icon_size : 20;
            int ix = draw_x + (draw_w - icon_s) / 2;
            int iy = draw_y + (draw_h - icon_s) / 2;

            ImageView temp_img(!res_path.empty() ? res_path : m_icon);
            temp_img.set_target_size(icon_s);
            temp_img.draw(cr, Rect(ix, iy, icon_s, icon_s));
        } else {
            Color fg = m_custom_icon_color ? m_icon_color : config->colors.on_surface;
            if (m_pressed) fg = config->colors.primary;

        PangoLayout* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, m_icon.c_str(), -1);

        std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
        if (font_family.empty()) font_family = "Sans";
        int font_size = m_icon_size > 0 ? m_icon_size : 14;

        std::string font_desc = font_family + " " + std::to_string(font_size);
        PangoFontDescription* desc = pango_font_description_from_string(font_desc.c_str());
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        int text_w = 0, text_h = 0;
        pango_layout_get_pixel_size(layout, &text_w, &text_h);

        int tx = draw_x + (draw_w - text_w) / 2;
        int ty = draw_y + (draw_h - text_h) / 2;

        cairo_move_to(cr, tx, ty);
        cairo_set_source_rgba(cr, fg.r, fg.g, fg.b, fg.a);
        pango_cairo_show_layout(cr, layout);

        g_object_unref(layout);
        }
    }

    cairo_restore(cr);
}

bool ImageButton::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    bool contains = (lx >= bounds.x && lx <= bounds.x + bounds.width &&
                     ly >= bounds.y && ly <= bounds.y + bounds.height);

    if (pressed) {
        if (contains) {
            m_pressed = true;
            if (m_window) m_window->schedule_redraw();
            return true;
        }
    } else {
        if (m_pressed) {
            m_pressed = false;
            if (m_window) m_window->schedule_redraw();
            if (contains && m_on_click) {
                m_on_click();
            }
            return true;
        }
    }
    return false;
}

bool ImageButton::on_mouse_move(int lx, int ly, const Rect& bounds) {
    bool contains = (lx >= bounds.x && lx <= bounds.x + bounds.width &&
                     ly >= bounds.y && ly <= bounds.y + bounds.height);

    if (contains != m_hovered) {
        m_hovered = contains;
        if (m_window) m_window->schedule_redraw();
        return true;
    }
    return false;
}

} // namespace miqu
