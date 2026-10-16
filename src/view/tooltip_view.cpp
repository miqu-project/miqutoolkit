#include "miqutoolkit/view/tooltip_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/view/view_group.hpp"
#include "miqutoolkit/core/config.hpp"
#include <pango/pangocairo.h>
#include <algorithm>
#include <memory>

namespace miqu {

namespace {

struct CairoSurfaceDeleter {
    void operator()(cairo_surface_t* s) const { if (s) cairo_surface_destroy(s); }
};
struct CairoDeleter {
    void operator()(cairo_t* cr) const { if (cr) cairo_destroy(cr); }
};
struct GObjectDeleter {
    void operator()(void* obj) const { if (obj) g_object_unref(obj); }
};
struct PangoFontDescDeleter {
    void operator()(PangoFontDescription* desc) const { if (desc) pango_font_description_free(desc); }
};

using UniqueCairoSurface = std::unique_ptr<cairo_surface_t, CairoSurfaceDeleter>;
using UniqueCairo = std::unique_ptr<cairo_t, CairoDeleter>;
using UniquePangoLayout = std::unique_ptr<PangoLayout, GObjectDeleter>;
using UniquePangoFontDesc = std::unique_ptr<PangoFontDescription, PangoFontDescDeleter>;

} // anonymous namespace

TooltipView::TooltipView() = default;

TooltipView::TooltipView(std::string text, std::string shortcut)
    : m_text(std::move(text)), m_shortcut(std::move(shortcut)) {}

void TooltipView::set_text(std::string text) {
    if (m_text != text) {
        m_text = std::move(text);
        request_redraw();
    }
}

void TooltipView::set_shortcut(std::string shortcut) {
    if (m_shortcut != shortcut) {
        m_shortcut = std::move(shortcut);
        request_redraw();
    }
}

Size TooltipView::measure_size() const {
    return measure_size(0);
}

Size TooltipView::measure_size(int avail_width) const {
    (void)avail_width;
    if (m_text.empty() && m_shortcut.empty()) {
        return Size(0, 0);
    }

    UniqueCairoSurface surf(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1));
    if (!surf) return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);

    UniqueCairo cr(cairo_create(surf.get()));
    if (!cr) return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);

    auto config = Config::get();
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
    int font_size = config->metrics.caption_size > 0 ? config->metrics.caption_size : 11;
    std::string desc_str = font_family + " " + std::to_string(font_size);

    int tw = 0, th = 0;
    if (!m_text.empty()) {
        UniquePangoLayout layout(pango_cairo_create_layout(cr.get()));
        if (layout) {
            pango_layout_set_text(layout.get(), m_text.c_str(), -1);
            UniquePangoFontDesc desc(pango_font_description_from_string(desc_str.c_str()));
            if (desc) {
                pango_layout_set_font_description(layout.get(), desc.get());
            }
            pango_layout_get_pixel_size(layout.get(), &tw, &th);
        }
    }

    int sw = 0, sh = 0;
    if (!m_shortcut.empty()) {
        UniquePangoLayout s_layout(pango_cairo_create_layout(cr.get()));
        if (s_layout) {
            pango_layout_set_text(s_layout.get(), m_shortcut.c_str(), -1);
            int s_font_size = std::max(8, font_size - 1);
            std::string s_desc_str = font_family + " " + std::to_string(s_font_size);
            UniquePangoFontDesc desc(pango_font_description_from_string(s_desc_str.c_str()));
            if (desc) {
                pango_layout_set_font_description(s_layout.get(), desc.get());
            }
            pango_layout_get_pixel_size(s_layout.get(), &sw, &sh);
        }
    }

    int gap = (!m_text.empty() && !m_shortcut.empty()) ? 8 : 0;
    int extra_badge_pad = (!m_shortcut.empty()) ? 8 : 0;
    int total_w = tw + sw + gap + extra_badge_pad + m_padding.left + m_padding.right;
    int total_h = std::max(th, sh) + m_padding.top + m_padding.bottom;
    return Size(total_w, total_h);
}

void TooltipView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || (m_text.empty() && m_shortcut.empty()) || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    int radius = (m_corner_radius >= 0) ? m_corner_radius : 6;
    float opacity = (config->metrics.tooltip_opacity >= 0.0f) ? config->metrics.tooltip_opacity : 0.90f;
    Color bg = config->colors.surface_variant.with_alpha(opacity);
    Color text_color = (config->colors.on_surface_variant.a > 0.0f) ? config->colors.on_surface_variant : config->colors.on_surface;

    // 1. Ambient drop shadow if elevated
    double elev = (m_elevation >= 0.0) ? m_elevation : config->metrics.tooltip_elevation;
    if (elev > 0.0) {
        CardView::draw_elevation_shadow(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius, elev);
    }

    // 2. Rounded background fill (no outline)
    cairo_save(cr);
    ViewGroup::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
    cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
    cairo_fill(cr);
    cairo_restore(cr);

    // 3. Content Typography & Shortcut rendering
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
    int font_size = config->metrics.caption_size > 0 ? config->metrics.caption_size : 11;
    std::string desc_str = font_family + " " + std::to_string(font_size);

    int tw = 0, th = 0;
    UniquePangoLayout text_layout(pango_cairo_create_layout(cr));
    if (text_layout && !m_text.empty()) {
        pango_layout_set_text(text_layout.get(), m_text.c_str(), -1);
        UniquePangoFontDesc desc(pango_font_description_from_string(desc_str.c_str()));
        if (desc) {
            pango_layout_set_font_description(text_layout.get(), desc.get());
        }
        pango_layout_get_pixel_size(text_layout.get(), &tw, &th);
    }

    int sw = 0, sh = 0;
    UniquePangoLayout s_layout(pango_cairo_create_layout(cr));
    if (s_layout && !m_shortcut.empty()) {
        pango_layout_set_text(s_layout.get(), m_shortcut.c_str(), -1);
        int s_font_size = std::max(8, font_size - 1);
        std::string s_desc_str = font_family + " " + std::to_string(s_font_size);
        UniquePangoFontDesc s_desc(pango_font_description_from_string(s_desc_str.c_str()));
        if (s_desc) {
            pango_layout_set_font_description(s_layout.get(), s_desc.get());
        }
        pango_layout_get_pixel_size(s_layout.get(), &sw, &sh);
    }

    int gap = (!m_text.empty() && !m_shortcut.empty()) ? 8 : 0;
    int extra_badge_pad = (!m_shortcut.empty()) ? 8 : 0;
    int content_w = tw + sw + gap + extra_badge_pad;
    int start_x = bounds.x + (bounds.width - content_w) / 2;

    if (text_layout && !m_text.empty()) {
        int tx = start_x;
        int ty = bounds.y + (bounds.height - th) / 2;
        cairo_save(cr);
        cairo_set_source_rgba(cr, text_color.r, text_color.g, text_color.b, text_color.a);
        cairo_move_to(cr, tx, ty);
        pango_cairo_show_layout(cr, text_layout.get());
        cairo_restore(cr);
    }

    if (s_layout && !m_shortcut.empty()) {
        int badge_x = start_x + (tw > 0 ? tw + gap : 0);
        int badge_w = sw + 8;
        int badge_h = sh + 2;
        int badge_y = bounds.y + (bounds.height - badge_h) / 2;

        cairo_save(cr);
        ViewGroup::draw_rounded_rect(cr, badge_x, badge_y, badge_w, badge_h, 4);
        cairo_set_source_rgba(cr, text_color.r, text_color.g, text_color.b, 0.12);
        cairo_fill(cr);
        cairo_restore(cr);

        cairo_save(cr);
        cairo_set_source_rgba(cr, text_color.r, text_color.g, text_color.b, text_color.a * 0.85f);
        cairo_move_to(cr, badge_x + 4, badge_y + 1);
        pango_cairo_show_layout(cr, s_layout.get());
        cairo_restore(cr);
    }
}

} // namespace miqu
