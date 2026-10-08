#include "miqutoolkit/view/grid_item_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <algorithm>
#include <unordered_map>
#include <mutex>

namespace miqu {

static std::mutex s_font_desc_mutex;
static std::unordered_map<std::string, PangoFontDescription*> s_font_desc_cache;

static PangoFontDescription* get_font_desc(const std::string& font_family, int font_size, bool bold = false) {
    std::string key = font_family + (bold ? " Bold " : " ") + std::to_string(font_size);
    std::lock_guard<std::mutex> lock(s_font_desc_mutex);
    auto it = s_font_desc_cache.find(key);
    if (it != s_font_desc_cache.end()) {
        return it->second;
    }
    PangoFontDescription* desc = pango_font_description_from_string(key.c_str());
    s_font_desc_cache[key] = desc;
    return desc;
}

GridItemView::GridItemView(std::string title, std::string icon_source, bool is_image)
    : m_title(std::move(title)), m_icon_source(std::move(icon_source)), m_is_image(is_image) {
    m_icon_view.set_image_resource(m_icon_source);
}

GridItemView::~GridItemView() {
    if (m_title_layout) {
        g_object_unref(static_cast<PangoLayout*>(m_title_layout));
        m_title_layout = nullptr;
    }
    if (m_sub_layout) {
        g_object_unref(static_cast<PangoLayout*>(m_sub_layout));
        m_sub_layout = nullptr;
    }
}

void GridItemView::set_title(std::string title) {
    if (m_title != title) {
        m_title = std::move(title);
        m_title_dirty = true;
    }
}

void GridItemView::set_title_bold(bool bold) {
    if (m_title_bold != bold) {
        m_title_bold = bold;
        m_title_dirty = true;
    }
}

void GridItemView::set_highlight_title(bool highlight) {
    if (m_highlight_title != highlight) {
        m_highlight_title = highlight;
    }
}

void GridItemView::set_subtitle(std::string subtitle) {
    if (m_subtitle != subtitle) {
        m_subtitle = std::move(subtitle);
        m_sub_dirty = true;
    }
}

void GridItemView::set_icon_source(std::string source) {
    if (m_icon_source != source) {
        m_icon_source = std::move(source);
        m_icon_view.set_image_resource(m_icon_source);
    }
}

void GridItemView::set_is_image(bool is_image) {
    m_is_image = is_image;
}

void GridItemView::set_quality_mode(ImageQuality quality) {
    m_quality = quality;
    m_icon_view.set_quality_mode(quality);
}

void GridItemView::draw(cairo_t* cr, const Rect& bounds) {
    if (!cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
    int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 11;
    int horiz_padding = 8;
    int text_max_w = std::max(0, bounds.width - horiz_padding * 2);

    bool width_changed = (text_max_w != m_last_text_max_w);
    m_last_text_max_w = text_max_w;

    // 1. Prepare / Update Title Layout
    PangoLayout* title_layout = static_cast<PangoLayout*>(m_title_layout);
    if (!title_layout) {
        title_layout = pango_cairo_create_layout(cr);
        m_title_layout = title_layout;
        m_title_dirty = true;
    }

    if (m_title_dirty || width_changed) {
        pango_layout_set_text(title_layout, m_title.c_str(), -1);
        PangoFontDescription* title_desc = get_font_desc(font_family, font_size, m_title_bold);
        pango_layout_set_font_description(title_layout, title_desc);
        pango_layout_set_alignment(title_layout, PANGO_ALIGN_CENTER);
        pango_layout_set_width(title_layout, text_max_w * PANGO_SCALE);
        pango_layout_set_ellipsize(title_layout, PANGO_ELLIPSIZE_END);
        pango_layout_get_pixel_size(title_layout, &m_cached_title_w, &m_cached_title_h);
        m_title_dirty = false;
    }

    int title_h = m_cached_title_h;

    // 2. Prepare / Update Subtitle Layout
    bool has_subtitle = !m_subtitle.empty() && !m_is_image;
    PangoLayout* sub_layout = static_cast<PangoLayout*>(m_sub_layout);

    if (has_subtitle) {
        if (!sub_layout) {
            sub_layout = pango_cairo_create_layout(cr);
            m_sub_layout = sub_layout;
            m_sub_dirty = true;
        }

        if (m_sub_dirty || width_changed) {
            pango_layout_set_text(sub_layout, m_subtitle.c_str(), -1);
            int sub_size = std::max(6, font_size - 2);
            PangoFontDescription* sub_desc = get_font_desc(font_family, sub_size);
            pango_layout_set_font_description(sub_layout, sub_desc);
            pango_layout_set_alignment(sub_layout, PANGO_ALIGN_CENTER);
            pango_layout_set_width(sub_layout, text_max_w * PANGO_SCALE);
            pango_layout_set_ellipsize(sub_layout, PANGO_ELLIPSIZE_END);
            pango_layout_get_pixel_size(sub_layout, &m_cached_sub_w, &m_cached_sub_h);
            m_sub_dirty = false;
        }
    }

    int sub_h = m_cached_sub_h;
    int title_y = 0;
    int sub_y = 0;

    if (m_is_image) {
        // --- Image Thumbnail Mode ---
        int pad_x = 8;
        int pad_top = 8;
        int pad_bottom = 6;
        int gap = 6;

        int img_w = bounds.width - pad_x * 2;
        int img_h = std::max(20, bounds.height - pad_top - gap - title_h - pad_bottom);

        Rect img_rect(bounds.x + pad_x, bounds.y + pad_top, img_w, img_h);
        m_icon_view.set_target_size(std::max(img_w, img_h));
        m_icon_view.set_fit_mode(FitMode::Cover);
        m_icon_view.set_quality_mode(m_quality);
        m_icon_view.set_corner_radius(m_corner_radius);
        m_icon_view.draw(cr, img_rect);

        title_y = bounds.y + pad_top + img_h + gap;
    } else {
        // --- Standard App Icon Mode ---
        int icon_size = has_subtitle ? 42 : 48;
        int icon_title_gap = has_subtitle ? 6 : 10;
        int title_sub_gap = 4;

        int total_content_h = icon_size + icon_title_gap + title_h + (has_subtitle ? (title_sub_gap + sub_h) : 0);
        int top_offset = std::max(6, (bounds.height - total_content_h) / 2);

        Rect icon_rect(bounds.x + (bounds.width - icon_size) / 2, bounds.y + top_offset, icon_size, icon_size);
        m_icon_view.set_target_size(icon_size);
        m_icon_view.set_quality_mode(m_quality);
        m_icon_view.draw(cr, icon_rect);

        title_y = bounds.y + top_offset + icon_size + icon_title_gap;
        if (has_subtitle) {
            sub_y = title_y + title_h + title_sub_gap;
        }
    }

    // 3. Render Title
    cairo_move_to(cr, bounds.x + horiz_padding, title_y);
    if (m_highlight_title) {
        cairo_set_source_rgba(cr, config->colors.primary.r,
                                  config->colors.primary.g,
                                  config->colors.primary.b,
                                  1.0f);
    } else {
        cairo_set_source_rgba(cr, config->colors.on_surface.r,
                                  config->colors.on_surface.g,
                                  config->colors.on_surface.b,
                                  config->colors.on_surface.a);
    }
    pango_cairo_show_layout(cr, title_layout);

    // 4. Render Subtitle
    if (has_subtitle && sub_layout) {
        cairo_move_to(cr, bounds.x + horiz_padding, sub_y);
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
    }
}

} // namespace miqu
