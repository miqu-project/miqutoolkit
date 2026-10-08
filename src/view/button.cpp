#include "miqutoolkit/view/button.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <iostream>

namespace miqu {

Size Button::measure_size() const {
    cairo_surface_t* temp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* cr = cairo_create(temp_surf);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, m_text.c_str(), -1);

    auto config = Config::get();
    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    bool is_bold = m_font_bold || (m_style == ButtonStyle::Primary);
    if (is_bold) font_desc_str += " Bold";

    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(temp_surf);

    int icon_space = m_icon.empty() ? 0 : (font_size > 0 ? font_size + 12 : 24);
    int pad_h = m_padding.left + m_padding.right > 0 ? (m_padding.left + m_padding.right) : 24;
    int pad_v = m_padding.top + m_padding.bottom > 0 ? (m_padding.top + m_padding.bottom) : 12;

    int total_w = text_w + icon_space + pad_h;
    int total_h = std::max(text_h, 20) + pad_v;

    return Size(total_w, total_h);
}

void Button::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_bounds = bounds;
    auto config = Config::get();

    int draw_x = bounds.x;
    int draw_y = bounds.y;
    int draw_w = bounds.width;
    int draw_h = bounds.height;

    if (draw_w <= 0 || draw_h <= 0) return;

    Color bg_color = Color::transparent();
    Color fg_color = config->colors.on_surface;
    Color border_color = Color::transparent();
    double border_width = 0.0;

    if (m_use_custom_colors) {
        bg_color = m_custom_bg;
        fg_color = m_custom_fg;
    } else if (m_selected) {
        bg_color = config->colors.primary_container;
        fg_color = config->colors.on_primary_container;
        border_color = config->colors.primary;
        border_width = 1.0;
    } else {
        switch (m_style) {
            case ButtonStyle::Primary:
                if (m_pressed) {
                    bg_color = config->colors.primary.with_alpha(0.70f);
                } else if (m_hovered) {
                    bg_color = config->colors.primary.with_alpha(0.88f);
                } else {
                    bg_color = config->colors.primary;
                }
                fg_color = config->colors.on_primary;
                break;

            case ButtonStyle::Outlined:
                if (m_pressed) {
                    bg_color = config->colors.primary.with_alpha(0.20f);
                } else if (m_hovered) {
                    bg_color = config->colors.surface_variant.with_alpha(0.60f);
                } else {
                    bg_color = Color::transparent();
                }
                fg_color = config->colors.primary;
                border_color = config->colors.outline;
                border_width = 1.0;
                break;

            case ButtonStyle::Flat:
                if (m_pressed) {
                    bg_color = config->colors.primary.with_alpha(0.25f);
                    fg_color = config->colors.primary;
                } else if (m_hovered) {
                    bg_color = config->colors.surface_variant.with_alpha(0.60f);
                    fg_color = config->colors.on_surface;
                } else {
                    bg_color = Color::transparent();
                    fg_color = config->colors.on_surface;
                }
                break;

            case ButtonStyle::Standard:
            default:
                if (m_pressed) {
                    bg_color = config->colors.primary.with_alpha(0.30f);
                    fg_color = config->colors.primary;
                    border_color = config->colors.outline;
                    border_width = 1.0;
                } else if (m_hovered) {
                    bg_color = config->colors.surface_variant;
                    fg_color = config->colors.on_surface;
                    border_color = config->colors.outline;
                    border_width = 1.0;
                } else {
                    bg_color = config->colors.surface_variant.with_alpha(0.65f);
                    fg_color = config->colors.on_surface;
                    border_color = config->colors.outline_variant;
                    border_width = 1.0;
                }
                break;
        }
    }

    int radius = (m_corner_radius >= 0) ? m_corner_radius : config->metrics.corner_radius;

    cairo_save(cr);

    // Background
    if (bg_color.a > 0.0f) {
        CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, radius);
        cairo_set_source_rgba(cr, bg_color.r, bg_color.g, bg_color.b, bg_color.a);
        cairo_fill(cr);
    }

    // Border
    if (border_width > 0.0 && border_color.a > 0.0f) {
        CardView::draw_rounded_rect(cr, draw_x + 0.5, draw_y + 0.5, draw_w - 1.0, draw_h - 1.0, std::max(0, radius - 1));
        cairo_set_source_rgba(cr, border_color.r, border_color.g, border_color.b, border_color.a);
        cairo_set_line_width(cr, border_width);
        cairo_stroke(cr);
    }

    int pad_l = m_padding.left > 0 ? m_padding.left : 12;
    int pad_r = m_padding.right > 0 ? m_padding.right : 12;
    int avail_content_w = std::max(0, draw_w - pad_l - pad_r);

    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;
    int icon_size = font_size + 4;
    int icon_w = m_icon.empty() ? 0 : icon_size;
    int gap = (!m_icon.empty() && !m_text.empty()) ? 8 : 0;

    PangoLayout* text_layout = nullptr;
    int text_w = 0, text_h = 0;
    if (!m_text.empty()) {
        text_layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(text_layout, m_text.c_str(), -1);

        std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
        if (font_family.empty()) font_family = "Sans";

        std::string font_desc_str = font_family + " " + std::to_string(font_size);
        bool is_bold = m_font_bold || (m_style == ButtonStyle::Primary);
        if (is_bold) font_desc_str += " Bold";

        PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
        pango_layout_set_font_description(text_layout, desc);
        pango_font_description_free(desc);

        int unconstrained_w = 0, unconstrained_h = 0;
        pango_layout_get_pixel_size(text_layout, &unconstrained_w, &unconstrained_h);

        int max_text_w = std::max(0, avail_content_w - icon_w - gap);
        if (unconstrained_w > max_text_w) {
            pango_layout_set_width(text_layout, max_text_w * PANGO_SCALE);
            pango_layout_set_ellipsize(text_layout, PANGO_ELLIPSIZE_END);
            pango_layout_get_pixel_size(text_layout, &text_w, &text_h);
        } else {
            text_w = unconstrained_w;
            text_h = unconstrained_h;
        }
    }

    int total_content_w = icon_w + gap + text_w;
    int content_start_x = draw_x + pad_l + std::max(0, (avail_content_w - total_content_w) / 2);

    if (!m_icon.empty()) {
        int icon_x = content_start_x;
        int icon_y = draw_y + (draw_h - icon_size) / 2;
        Rect icon_rect(icon_x, icon_y, icon_size, icon_size);

        ImageView img(m_icon);
        img.set_target_size(icon_size);
        img.draw(cr, icon_rect);

        content_start_x += icon_w + gap;
    }

    if (text_layout) {
        double label_y = draw_y + (draw_h - text_h) / 2.0;
        cairo_move_to(cr, content_start_x, label_y);
        cairo_set_source_rgba(cr, fg_color.r, fg_color.g, fg_color.b, fg_color.a);
        pango_cairo_show_layout(cr, text_layout);
        g_object_unref(text_layout);
    }

    cairo_restore(cr);
}

bool Button::on_mouse_move(int lx, int ly, const Rect& bounds) {
    bool hovered = bounds.contains(Point(lx, ly));
    if (hovered != m_hovered) {
        m_hovered = hovered;
        if (m_window) m_window->schedule_redraw();
        return true;
    }
    return false;
}

bool Button::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    bool contains = bounds.contains(Point(lx, ly));
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

} // namespace miqu
