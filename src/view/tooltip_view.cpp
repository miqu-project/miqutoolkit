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

TooltipView::TooltipView(std::string text)
    : m_text(std::move(text)) {}

void TooltipView::set_text(std::string text) {
    if (m_text != text) {
        m_text = std::move(text);
        request_redraw();
    }
}

Size TooltipView::measure_size() const {
    return measure_size(0);
}

Size TooltipView::measure_size(int avail_width) const {
    (void)avail_width;
    if (m_text.empty()) {
        return Size(0, 0);
    }

    UniqueCairoSurface surf(cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1));
    if (!surf) return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);

    UniqueCairo cr(cairo_create(surf.get()));
    if (!cr) return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);

    UniquePangoLayout layout(pango_cairo_create_layout(cr.get()));
    if (!layout) return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);

    pango_layout_set_text(layout.get(), m_text.c_str(), -1);

    auto config = Config::get();
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
    int font_size = config->metrics.caption_size > 0 ? config->metrics.caption_size : 11;
    std::string desc_str = font_family + " " + std::to_string(font_size);

    UniquePangoFontDesc desc(pango_font_description_from_string(desc_str.c_str()));
    if (desc) {
        pango_layout_set_font_description(layout.get(), desc.get());
    }

    int tw = 0, th = 0;
    pango_layout_get_pixel_size(layout.get(), &tw, &th);

    int total_w = tw + m_padding.left + m_padding.right;
    int total_h = th + m_padding.top + m_padding.bottom;
    return Size(total_w, total_h);
}

void TooltipView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || m_text.empty() || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    int radius = (m_corner_radius >= 0) ? m_corner_radius : 6;
    Color bg = (config->colors.surface_variant.a > 0.0f) ? config->colors.surface_variant : config->colors.surface;
    Color stroke = (config->colors.outline_variant.a > 0.0f) ? config->colors.outline_variant : Color::rgba(1.0f, 1.0f, 1.0f, 0.15f);
    Color text_color = (config->colors.on_surface_variant.a > 0.0f) ? config->colors.on_surface_variant : config->colors.on_surface;

    // 1. Ambient drop shadow if elevated
    if (m_elevation > 0.0) {
        CardView::draw_elevation_shadow(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius, m_elevation);
    }

    // 2. Rounded background fill
    cairo_save(cr);
    ViewGroup::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
    cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
    cairo_fill(cr);
    cairo_restore(cr);

    // 3. Subtle outline border
    cairo_save(cr);
    ViewGroup::draw_rounded_rect(cr, bounds.x + 0.5, bounds.y + 0.5, bounds.width - 1.0, bounds.height - 1.0, std::max(0, radius - 1));
    cairo_set_source_rgba(cr, stroke.r, stroke.g, stroke.b, stroke.a);
    cairo_set_line_width(cr, 1.0);
    cairo_stroke(cr);
    cairo_restore(cr);

    // 4. Centered text layout
    UniquePangoLayout layout(pango_cairo_create_layout(cr));
    if (layout) {
        pango_layout_set_text(layout.get(), m_text.c_str(), -1);

        std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
        int font_size = config->metrics.caption_size > 0 ? config->metrics.caption_size : 11;
        std::string desc_str = font_family + " " + std::to_string(font_size);

        UniquePangoFontDesc desc(pango_font_description_from_string(desc_str.c_str()));
        if (desc) {
            pango_layout_set_font_description(layout.get(), desc.get());
        }

        int tw = 0, th = 0;
        pango_layout_get_pixel_size(layout.get(), &tw, &th);

        int tx = bounds.x + (bounds.width - tw) / 2;
        int ty = bounds.y + (bounds.height - th) / 2;

        cairo_save(cr);
        cairo_set_source_rgba(cr, text_color.r, text_color.g, text_color.b, text_color.a);
        cairo_move_to(cr, tx, ty);
        pango_cairo_show_layout(cr, layout.get());
        cairo_restore(cr);
    }
}

} // namespace miqu
