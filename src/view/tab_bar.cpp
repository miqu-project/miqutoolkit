#include "miqutoolkit/view/tab_bar.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <algorithm>
#include <iostream>

namespace miqu {

TabBar::TabBar() {
    m_padding = Padding(4, 4, 4, 4);
}

void TabBar::set_tabs(const std::vector<std::string>& tabs) {
    m_tabs = tabs;
    if (m_selected_index >= static_cast<int>(m_tabs.size())) {
        m_selected_index = m_tabs.empty() ? 0 : static_cast<int>(m_tabs.size()) - 1;
    }
    if (m_window) m_window->schedule_redraw();
}

void TabBar::add_tab(const std::string& label) {
    m_tabs.push_back(label);
    if (m_window) m_window->schedule_redraw();
}

void TabBar::set_tab_text(int index, const std::string& text) {
    if (index >= 0 && index < static_cast<int>(m_tabs.size())) {
        m_tabs[index] = text;
        if (m_window) m_window->schedule_redraw();
    }
}

void TabBar::set_selected_index(int index) {
    if (index >= 0 && index < static_cast<int>(m_tabs.size()) && index != m_selected_index) {
        m_selected_index = index;
        if (m_on_tab_selected) {
            m_on_tab_selected(m_selected_index);
        }
        if (m_window) m_window->schedule_redraw();
    }
}

int TabBar::get_tab_index_at(int lx, int ly, const Rect& bounds) const {
    if (m_tabs.empty() || bounds.width <= 0 || bounds.height <= 0) return -1;
    if (!bounds.contains(Point(lx, ly))) return -1;

    Rect content_rect = get_content_rect(bounds);
    if (!content_rect.contains(Point(lx, ly))) return -1;

    if (m_equal_widths) {
        int tab_w = content_rect.width / static_cast<int>(m_tabs.size());
        if (tab_w <= 0) return -1;
        int idx = (lx - content_rect.x) / tab_w;
        return std::clamp(idx, 0, static_cast<int>(m_tabs.size()) - 1);
    }
    return -1;
}

Size TabBar::measure_size() const {
    auto config = Config::get();
    int font_sz = (m_font_size > 0) ? m_font_size : config->metrics.font_size;
    if (font_sz <= 0) font_sz = 11;

    int total_w = m_padding.left + m_padding.right;
    for (const auto& tab : m_tabs) {
        total_w += static_cast<int>(tab.size() * (font_sz * 0.65f)) + 24;
    }
    int h = font_sz + 18 + m_padding.top + m_padding.bottom;
    return Size(std::max(120, total_w), std::max(34, h));
}

void TabBar::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0 || m_tabs.empty()) return;

    auto config = Config::get();
    Rect content_rect = get_content_rect(bounds);
    if (content_rect.width <= 0 || content_rect.height <= 0) return;

    int radius = (m_corner_radius >= 0) ? m_corner_radius : config->metrics.corner_radius;

    cairo_save(cr);

    // 1. Segmented background track
    if (m_style == TabBarStyle::Segmented) {
        CardView::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  0.45f);
        cairo_fill(cr);

        CardView::draw_rounded_rect(cr, bounds.x + 0.5, bounds.y + 0.5, bounds.width - 1.0, bounds.height - 1.0, std::max(0, radius - 1));
        cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                  config->colors.outline_variant.g,
                                  config->colors.outline_variant.b,
                                  0.60f);
        cairo_set_line_width(cr, 1.0);
        cairo_stroke(cr);
    }

    size_t count = m_tabs.size();
    int tab_w = content_rect.width / static_cast<int>(count);
    int inner_radius = std::max(2, radius - 2);

    int font_sz = (m_font_size > 0) ? m_font_size : config->metrics.font_size;
    if (font_sz <= 0) font_sz = 11;
    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";

    for (size_t i = 0; i < count; ++i) {
        int tx = content_rect.x + static_cast<int>(i) * tab_w;
        int tw = (i == count - 1) ? (content_rect.x + content_rect.width - tx) : tab_w;
        int ty = content_rect.y;
        int th = content_rect.height;

        bool is_selected = (static_cast<int>(i) == m_selected_index);
        bool is_hovered = (static_cast<int>(i) == m_hovered_index);

        // Active selection pill or underline
        if (m_style == TabBarStyle::Segmented) {
            if (is_selected) {
                CardView::draw_rounded_rect(cr, tx, ty, tw, th, inner_radius);
                cairo_set_source_rgba(cr, config->colors.primary_container.r,
                                          config->colors.primary_container.g,
                                          config->colors.primary_container.b,
                                          config->colors.primary_container.a);
                cairo_fill(cr);

                CardView::draw_rounded_rect(cr, tx + 0.5, ty + 0.5, tw - 1.0, th - 1.0, std::max(0, inner_radius - 1));
                cairo_set_source_rgba(cr, config->colors.primary.r,
                                          config->colors.primary.g,
                                          config->colors.primary.b,
                                          0.35f);
                cairo_set_line_width(cr, 1.0);
                cairo_stroke(cr);
            } else if (is_hovered) {
                CardView::draw_rounded_rect(cr, tx, ty, tw, th, inner_radius);
                cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                          config->colors.surface_variant.g,
                                          config->colors.surface_variant.b,
                                          0.50f);
                cairo_fill(cr);
            }
        } else { // Underline style
            if (is_selected) {
                int line_h = 3;
                cairo_rectangle(cr, tx + 8, ty + th - line_h, tw - 16, line_h);
                cairo_set_source_rgba(cr, config->colors.primary.r,
                                          config->colors.primary.g,
                                          config->colors.primary.b,
                                          config->colors.primary.a);
                cairo_fill(cr);
            }
        }

        // Tab Label
        PangoLayout* layout = pango_cairo_create_layout(cr);
        pango_layout_set_text(layout, m_tabs[i].c_str(), -1);

        std::string font_desc = font_family + " " + std::to_string(font_sz);
        if (is_selected && m_bold_active) font_desc += " Bold";
        PangoFontDescription* desc = pango_font_description_from_string(font_desc.c_str());
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);

        pango_layout_set_width(layout, (tw - 8) * PANGO_SCALE);
        pango_layout_set_alignment(layout, PANGO_ALIGN_CENTER);
        pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

        int text_w = 0, text_h = 0;
        pango_layout_get_pixel_size(layout, &text_w, &text_h);

        double label_x = tx + 4.0;
        double label_y = ty + (th - text_h) / 2.0;

        cairo_move_to(cr, label_x, label_y);
        Color fg = is_selected ? config->colors.on_primary_container : config->colors.on_surface_variant;
        cairo_set_source_rgba(cr, fg.r, fg.g, fg.b, fg.a);
        pango_cairo_show_layout(cr, layout);

        g_object_unref(layout);
    }

    cairo_restore(cr);
}

bool TabBar::on_mouse_move(int lx, int ly, const Rect& bounds) {
    if (!is_visible() || m_tabs.empty()) return false;

    int old_hover = m_hovered_index;
    if (bounds.contains(Point(lx, ly))) {
        m_hovered_index = get_tab_index_at(lx, ly, bounds);
    } else {
        m_hovered_index = -1;
    }

    if (old_hover != m_hovered_index) {
        if (m_window) m_window->schedule_redraw();
    }
    return bounds.contains(Point(lx, ly));
}

bool TabBar::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left || !is_visible() || m_tabs.empty()) return false;

    bool contains = bounds.contains(Point(lx, ly));
    if (pressed) {
        if (!contains) return false;
        m_pressed_index = get_tab_index_at(lx, ly, bounds);
        if (m_window) m_window->schedule_redraw();
        return true;
    } else {
        int pressed_idx = m_pressed_index;
        m_pressed_index = -1;
        if (pressed_idx >= 0) {
            int release_idx = get_tab_index_at(lx, ly, bounds);
            if (release_idx >= 0 && release_idx == pressed_idx) {
                set_selected_index(release_idx);
            }
            if (m_window) m_window->schedule_redraw();
            return true;
        }
        if (contains) {
            return true;
        }
    }
    return false;
}

TabBarBuilder::TabBarBuilder() : m_view(std::make_shared<TabBar>()) {}

} // namespace miqu
