#include "miqutoolkit/view/text_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <algorithm>

namespace miqu {

static int resolve_font_size(int explicit_size, int heading_level, bool caption) {
    if (explicit_size > 0) return explicit_size;
    auto config = Config::get();
    if (heading_level == 1) return config->metrics.h1_size;
    if (heading_level == 2) return config->metrics.h2_size;
    if (heading_level == 3) return config->metrics.h3_size;
    if (caption) return config->metrics.caption_size;
    return config->metrics.font_size > 0 ? config->metrics.font_size : 11;
}

Size TextView::measure_size() const {
    return measure_size(-1);
}

Size TextView::measure_size(int avail_width) const {
    if (m_text.empty()) return Size(0, 0);

    cairo_surface_t* temp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
    cairo_t* cr = cairo_create(temp_surf);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, m_text.c_str(), -1);

    auto config = Config::get();
    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";

    int font_size = resolve_font_size(m_font_size, m_heading_level, m_caption);
    bool is_bold = m_bold || (m_heading_level >= 1 && m_heading_level <= 3);

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    if (is_bold) font_desc_str += " Bold";
    if (m_italic) font_desc_str += " Italic";

    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    if (m_line_spacing != 1.0f && m_line_spacing > 0.0f) {
        pango_layout_set_line_spacing(layout, m_line_spacing);
    }

    int target_w = -1;
    if (m_layout_params.width >= 0) {
        target_w = m_layout_params.width - m_padding.left - m_padding.right;
    } else if (avail_width >= 0) {
        target_w = avail_width - m_padding.left - m_padding.right;
    }

    if (m_multiline && target_w > 0) {
        pango_layout_set_width(layout, target_w * PANGO_SCALE);
        PangoWrapMode pango_wrap = PANGO_WRAP_WORD_CHAR;
        if (m_wrap_mode == WrapMode::Word) pango_wrap = PANGO_WRAP_WORD;
        else if (m_wrap_mode == WrapMode::Char) pango_wrap = PANGO_WRAP_CHAR;
        pango_layout_set_wrap(layout, pango_wrap);

        if (m_max_lines > 0) {
            pango_layout_set_height(layout, -m_max_lines);
            if (m_ellipsize && m_ellipsize_mode != EllipsizeMode::None) {
                pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
            }
        }
    }

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    g_object_unref(layout);
    cairo_destroy(cr);
    cairo_surface_destroy(temp_surf);

    int total_w = text_w + m_padding.left + m_padding.right;
    int total_h = text_h + m_padding.top + m_padding.bottom;
    return Size(total_w, total_h);
}

void TextView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || m_text.empty() || bounds.width <= 0 || bounds.height <= 0) return;

    Rect content_bounds = get_content_rect(bounds);
    int draw_x = content_bounds.x;
    int draw_y = content_bounds.y;
    int draw_w = content_bounds.width;
    int draw_h = content_bounds.height;

    if (draw_w <= 0 || draw_h <= 0) return;

    cairo_save(cr);

    // Clip to content rect
    cairo_rectangle(cr, draw_x, draw_y, draw_w, draw_h);
    cairo_clip(cr);

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, m_text.c_str(), -1);

    auto config = Config::get();
    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";

    int font_size = resolve_font_size(m_font_size, m_heading_level, m_caption);
    bool is_bold = m_bold || (m_heading_level >= 1 && m_heading_level <= 3);

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    if (is_bold) font_desc_str += " Bold";
    if (m_italic) font_desc_str += " Italic";

    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    if (m_line_spacing != 1.0f && m_line_spacing > 0.0f) {
        pango_layout_set_line_spacing(layout, m_line_spacing);
    }

    pango_layout_set_width(layout, draw_w * PANGO_SCALE);

    if (m_align == TextAlignment::Center) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
    } else if (m_align == TextAlignment::Right) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_RIGHT);
    } else if (m_align == TextAlignment::Justify) {
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
        pango_layout_set_justify(layout, TRUE);
    } else {
        pango_layout_set_alignment(layout, PANGO_ALIGN_LEFT);
    }

    if (m_multiline) {
        PangoWrapMode pango_wrap = PANGO_WRAP_WORD_CHAR;
        if (m_wrap_mode == WrapMode::Word) pango_wrap = PANGO_WRAP_WORD;
        else if (m_wrap_mode == WrapMode::Char) pango_wrap = PANGO_WRAP_CHAR;
        pango_layout_set_wrap(layout, pango_wrap);

        if (m_max_lines > 0) {
            pango_layout_set_height(layout, -m_max_lines);
            if (m_ellipsize && m_ellipsize_mode != EllipsizeMode::None) {
                pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
            } else {
                pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
            }
        } else {
            int unconstrained_w = 0, unconstrained_h = 0;
            pango_layout_get_pixel_size(layout, &unconstrained_w, &unconstrained_h);
            if (unconstrained_h > draw_h && m_ellipsize && m_ellipsize_mode != EllipsizeMode::None) {
                int lines = pango_layout_get_line_count(layout);
                if (lines > 1) {
                    int line_h = unconstrained_h / lines;
                    if (line_h > 0) {
                        int fitted = std::max(1, draw_h / line_h);
                        pango_layout_set_height(layout, -fitted);
                        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);
                    }
                }
            } else {
                pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
            }
        }
    } else {
        pango_layout_set_single_paragraph_mode(layout, TRUE);
        if (m_ellipsize && m_ellipsize_mode != EllipsizeMode::None) {
            PangoEllipsizeMode em = PANGO_ELLIPSIZE_END;
            if (m_ellipsize_mode == EllipsizeMode::Start) em = PANGO_ELLIPSIZE_START;
            else if (m_ellipsize_mode == EllipsizeMode::Middle) em = PANGO_ELLIPSIZE_MIDDLE;
            pango_layout_set_ellipsize(layout, em);
        } else {
            pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_NONE);
        }
    }

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    TextVerticalAlignment v_align = m_has_custom_vertical_align ?
        m_vertical_align :
        (m_multiline ? TextVerticalAlignment::Top : TextVerticalAlignment::Center);

    double baseline_y = draw_y;
    if (v_align == TextVerticalAlignment::Center) {
        if (draw_h > text_h) {
            baseline_y = draw_y + (draw_h - text_h) / 2.0;
        }
    } else if (v_align == TextVerticalAlignment::Bottom) {
        if (draw_h > text_h) {
            baseline_y = draw_y + (draw_h - text_h);
        }
    }

    cairo_move_to(cr, draw_x, baseline_y);
    Color text_col;
    if (m_has_custom_color) {
        text_col = m_color;
    } else if (m_muted) {
        text_col = config->colors.on_surface_variant;
    } else if (m_heading_level == 1) {
        text_col = config->colors.primary;
    } else if (m_heading_level == 2 || m_heading_level == 3) {
        text_col = config->colors.on_surface;
    } else if (m_caption) {
        text_col = config->colors.on_surface_variant;
    } else {
        text_col = config->colors.on_surface;
    }

    cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
    pango_cairo_show_layout(cr, layout);

    g_object_unref(layout);
    cairo_restore(cr);
}

} // namespace miqu
