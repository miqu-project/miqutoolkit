#include "miqutoolkit/view/list_item_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <algorithm>

namespace miqu {

ListItemView::ListItemView(std::string title, std::string subtitle, std::string icon_source)
    : m_title(std::move(title)), m_subtitle(std::move(subtitle)), m_icon_source(std::move(icon_source)) {}

void ListItemView::draw(cairo_t* cr, const Rect& bounds) {
    if (!cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
    int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 11;

    int pad_x = 12;
    int cur_x = bounds.x + pad_x;
    int right_edge = bounds.x + bounds.width - pad_x;

    // 1. Render Leading Icon (if present)
    if (!m_icon_source.empty()) {
        int icon_s = std::min(m_icon_size, std::max(16, bounds.height - 8));
        int icon_y = bounds.y + (bounds.height - icon_s) / 2;
        Rect icon_rect(cur_x, icon_y, icon_s, icon_s);

        ImageView img_view(m_icon_source);
        img_view.set_target_size(icon_s);
        img_view.set_fit_mode(m_is_image ? FitMode::Cover : FitMode::Contain);
        img_view.set_quality_mode(m_quality);
        img_view.set_corner_radius(m_icon_radius);
        img_view.draw(cr, icon_rect);

        cur_x += icon_s + 12;
    }

    // 2. Prepare Trailing Text (if present)
    PangoLayout* trailing_layout = nullptr;
    int trailing_w = 0, trailing_h = 0;
    if (!m_trailing_text.empty()) {
        trailing_layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(trailing_layout, m_trailing_text.c_str(), -1);
        int trail_size = std::max(6, font_size - 1);
        std::string trail_font = font_family + " " + std::to_string(trail_size);
        PangoFontDescription* trail_desc = pango_font_description_from_string(trail_font.c_str());
        pango_layout_set_font_description(trailing_layout, trail_desc);
        pango_font_description_free(trail_desc);
        pango_layout_get_pixel_size(trailing_layout, &trailing_w, &trailing_h);

        right_edge -= (trailing_w + 8);
    }

    // 3. Prepare Title & Subtitle Layouts
    int avail_text_w = std::max(20, right_edge - cur_x);

    PangoLayout* title_layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(title_layout, m_title.c_str(), -1);
    std::string title_font = font_family + " " + std::to_string(font_size);
    PangoFontDescription* title_desc = pango_font_description_from_string(title_font.c_str());
    pango_layout_set_font_description(title_layout, title_desc);
    pango_font_description_free(title_desc);
    pango_layout_set_width(title_layout, avail_text_w * PANGO_SCALE);
    pango_layout_set_ellipsize(title_layout, PANGO_ELLIPSIZE_END);

    int title_w = 0, title_h = 0;
    pango_layout_get_pixel_size(title_layout, &title_w, &title_h);

    bool has_sub = !m_subtitle.empty();
    PangoLayout* sub_layout = nullptr;
    int sub_w = 0, sub_h = 0;
    if (has_sub) {
        sub_layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(sub_layout, m_subtitle.c_str(), -1);
        int sub_size = std::max(6, font_size - 2);
        std::string sub_font = font_family + " " + std::to_string(sub_size);
        PangoFontDescription* sub_desc = pango_font_description_from_string(sub_font.c_str());
        pango_layout_set_font_description(sub_layout, sub_desc);
        pango_font_description_free(sub_desc);
        pango_layout_set_width(sub_layout, avail_text_w * PANGO_SCALE);
        pango_layout_set_ellipsize(sub_layout, PANGO_ELLIPSIZE_END);
        pango_layout_get_pixel_size(sub_layout, &sub_w, &sub_h);
    }

    int total_text_h = title_h + (has_sub ? (sub_h + 2) : 0);
    int text_y = bounds.y + (bounds.height - total_text_h) / 2;

    // 4. Render Title
    cairo_move_to(cr, cur_x, text_y);
    cairo_set_source_rgba(cr, config->colors.on_surface.r,
                              config->colors.on_surface.g,
                              config->colors.on_surface.b,
                              config->colors.on_surface.a);
    pango_cairo_show_layout(cr, title_layout);
    g_object_unref(title_layout);

    // 5. Render Subtitle
    if (has_sub && sub_layout) {
        cairo_move_to(cr, cur_x, text_y + title_h + 2);
        if (m_highlight_subtitle) {
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      1.0f);
        } else {
            cairo_set_source_rgba(cr, config->colors.on_surface_variant.r,
                                      config->colors.on_surface_variant.g,
                                      config->colors.on_surface_variant.b,
                                      0.75f);
        }
        pango_cairo_show_layout(cr, sub_layout);
        g_object_unref(sub_layout);
    }

    // 6. Render Trailing Text
    if (trailing_layout) {
        int trail_x = bounds.x + bounds.width - pad_x - trailing_w;
        int trail_y = bounds.y + (bounds.height - trailing_h) / 2;
        cairo_move_to(cr, trail_x, trail_y);
        if (m_highlight_trailing) {
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      1.0f);
        } else {
            cairo_set_source_rgba(cr, config->colors.on_surface_variant.r,
                                      config->colors.on_surface_variant.g,
                                      config->colors.on_surface_variant.b,
                                      0.65f);
        }
        pango_cairo_show_layout(cr, trailing_layout);
        g_object_unref(trailing_layout);
    }
}

} // namespace miqu
