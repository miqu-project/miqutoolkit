#include "miqutoolkit/view/spinner.hpp"
#include "miqutoolkit/view/popup_menu.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>

namespace miqu {

namespace {
using PangoLayoutPtr = std::unique_ptr<PangoLayout, decltype(&g_object_unref)>;
using PangoFontDescPtr = std::unique_ptr<PangoFontDescription, decltype(&pango_font_description_free)>;
} // anonymous namespace

Spinner::Spinner() = default;

Spinner::~Spinner() {
    close_dropdown();
}

void Spinner::set_items(std::vector<std::string> items) {
    m_items = std::move(items);
    if (m_selected_index < 0 && !m_items.empty()) {
        m_selected_index = 0;
    } else if (m_selected_index >= static_cast<int>(m_items.size())) {
        m_selected_index = m_items.empty() ? -1 : static_cast<int>(m_items.size()) - 1;
    }
    request_redraw();
}

void Spinner::add_item(std::string item) {
    m_items.push_back(std::move(item));
    if (m_selected_index < 0) {
        m_selected_index = 0;
    }
    request_redraw();
}

void Spinner::clear_items() {
    m_items.clear();
    m_selected_index = -1;
    close_dropdown();
    request_redraw();
}

void Spinner::set_selected_index(int index) {
    if (index >= -1 && index < static_cast<int>(m_items.size())) {
        if (m_selected_index != index) {
            m_selected_index = index;
            request_redraw();
            if (m_on_item_selected && m_selected_index >= 0) {
                m_on_item_selected(m_selected_index, m_items[m_selected_index]);
            }
        }
    }
}

std::string Spinner::get_selected_item() const {
    if (m_selected_index >= 0 && m_selected_index < static_cast<int>(m_items.size())) {
        return m_items[m_selected_index];
    }
    return "";
}

Size Spinner::measure_size() const {
    int w = (m_layout_params.width >= 0) ? m_layout_params.width : m_bounds.width;
    int h = (m_layout_params.height >= 0) ? m_layout_params.height : m_bounds.height;

    if (w > 0 && h > 0) {
        return Size(w, h);
    }

    auto config = Config::get();
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    int pad_h = (m_padding.left + m_padding.right > 0) ? (m_padding.left + m_padding.right) : 28;
    int pad_v = (m_padding.top + m_padding.bottom > 0) ? (m_padding.top + m_padding.bottom) : 14;

    int max_text_w = 80;
    for (const auto& it : m_items) {
        max_text_w = std::max(max_text_w, static_cast<int>(it.size()) * 8);
    }
    if (!m_prompt.empty()) {
        max_text_w = std::max(max_text_w, static_cast<int>(m_prompt.size()) * 8);
    }

    int measured_w = (w > 0) ? w : (max_text_w + 32 + pad_h);
    int measured_h = (h > 0) ? h : (std::max(24, font_size + 14) + pad_v);

    return Size(measured_w, measured_h);
}

Size Spinner::measure_size(int avail_width) const {
    if (m_layout_params.width == static_cast<int>(LayoutDimension::MatchParent) && avail_width > 0) {
        Size s = measure_size();
        return Size(avail_width, s.height);
    }
    return measure_size();
}

void Spinner::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_last_drawn_bounds = bounds;
    m_bounds = bounds;
    auto config = Config::get();

    int radius = (m_corner_radius >= 0) ? m_corner_radius : config->metrics.corner_radius;
    if (radius <= 0) radius = 8;

    cairo_save(cr);

    // 1. Background
    CardView::draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
    if (m_has_custom_bg) {
        cairo_set_source_rgba(cr, m_bg_color.r, m_bg_color.g, m_bg_color.b, m_bg_color.a);
    } else if (m_is_open) {
        cairo_set_source_rgba(cr, config->colors.surface.r,
                                  config->colors.surface.g,
                                  config->colors.surface.b,
                                  config->colors.surface.a);
    } else if (m_hovered) {
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  0.90f);
    } else {
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  0.65f);
    }
    cairo_fill(cr);

    // 2. Border
    CardView::draw_rounded_rect(cr, bounds.x + 0.5, bounds.y + 0.5, bounds.width - 1.0, bounds.height - 1.0, std::max(0, radius - 1));
    if (m_is_open) {
        cairo_set_source_rgba(cr, config->colors.primary.r,
                                  config->colors.primary.g,
                                  config->colors.primary.b,
                                  0.90f);
        cairo_set_line_width(cr, 1.5);
    } else {
        cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                  config->colors.outline_variant.g,
                                  config->colors.outline_variant.b,
                                  config->colors.outline_variant.a);
        cairo_set_line_width(cr, 1.0);
    }
    cairo_stroke(cr);

    // 3. Label Text
    std::string text_to_display = get_selected_item();
    bool is_placeholder = false;
    if (text_to_display.empty()) {
        text_to_display = m_prompt.empty() ? "Select item..." : m_prompt;
        is_placeholder = true;
    }

    std::string font_family = !m_font_family.empty() ? m_font_family : config->metrics.font_family;
    if (font_family.empty()) font_family = "Sans";
    int font_size = m_font_size > 0 ? m_font_size : config->metrics.font_size;
    if (font_size <= 0) font_size = 11;

    int pad_l = (m_padding.left > 0) ? m_padding.left : 14;
    int pad_r = (m_padding.right > 0) ? m_padding.right : 14;
    int arrow_space = 24;
    int text_max_w = std::max(0, bounds.width - pad_l - pad_r - arrow_space);

    PangoLayoutPtr layout(pango_cairo_create_layout(cr), g_object_unref);
    pango_layout_set_text(layout.get(), text_to_display.c_str(), -1);

    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    PangoFontDescPtr desc(pango_font_description_from_string(font_desc_str.c_str()), pango_font_description_free);
    pango_layout_set_font_description(layout.get(), desc.get());

    pango_layout_set_width(layout.get(), text_max_w * PANGO_SCALE);
    pango_layout_set_ellipsize(layout.get(), PANGO_ELLIPSIZE_END);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout.get(), &text_w, &text_h);

    int text_x = bounds.x + pad_l;
    int text_y = bounds.y + (bounds.height - text_h) / 2;

    Color text_col;
    if (m_has_custom_color) {
        text_col = m_text_color;
    } else {
        text_col = is_placeholder ? config->colors.on_surface_variant : config->colors.on_surface;
    }

    cairo_move_to(cr, text_x, text_y);
    cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
    pango_cairo_show_layout(cr, layout.get());

    // 4. Dropdown Chevron Arrow (▾)
    PangoLayoutPtr arrow_layout(pango_cairo_create_layout(cr), g_object_unref);
    pango_layout_set_text(arrow_layout.get(), m_is_open ? "▴" : "▾", -1);
    PangoFontDescPtr adesc(pango_font_description_from_string((font_family + " " + std::to_string(font_size + 1)).c_str()), pango_font_description_free);
    pango_layout_set_font_description(arrow_layout.get(), adesc.get());

    int aw = 0, ah = 0;
    pango_layout_get_pixel_size(arrow_layout.get(), &aw, &ah);
    int arrow_x = bounds.x + bounds.width - pad_r - aw;
    int arrow_y = bounds.y + (bounds.height - ah) / 2;

    Color arrow_col = m_is_open ? config->colors.primary : config->colors.on_surface_variant;
    cairo_move_to(cr, arrow_x, arrow_y);
    cairo_set_source_rgba(cr, arrow_col.r, arrow_col.g, arrow_col.b, arrow_col.a);
    pango_cairo_show_layout(cr, arrow_layout.get());

    cairo_restore(cr);
}

void Spinner::open_dropdown() {
    if (m_is_open || !m_window || m_items.empty()) return;

    m_is_open = true;

    m_popup_menu = std::make_shared<PopupMenu>();
    m_popup_menu->set_min_width(m_last_drawn_bounds.width);
    m_popup_menu->set_max_visible_items(m_max_visible_items);
    m_popup_menu->set_item_height(m_item_height);

    for (size_t i = 0; i < m_items.size(); ++i) {
        int idx = static_cast<int>(i);
        bool is_selected = (idx == m_selected_index);
        m_popup_menu->add_radio_item(m_items[i], 1, is_selected, [this, idx](bool) {
            handle_popup_selected(idx);
        });
    }

    m_popup_menu->set_selected_index(m_selected_index);
    m_popup_menu->set_on_dismiss([this]() {
        handle_popup_dismissed();
    });

    m_popup_menu->show_as_dropdown(this, PopupGravity::BottomStart, 0, 4);
    request_redraw();
}

void Spinner::close_dropdown() {
    if (!m_is_open) return;
    m_is_open = false;
    if (m_popup_menu) {
        auto menu = m_popup_menu;
        m_popup_menu = nullptr;
        menu->dismiss();
    }
    request_redraw();
}

void Spinner::handle_popup_selected(int index) {
    close_dropdown();
    set_selected_index(index);
}

void Spinner::handle_popup_dismissed() {
    m_is_open = false;
    m_popup_menu = nullptr;
    request_redraw();
}

bool Spinner::on_mouse_move(int lx, int ly, const Rect& bounds) {
    bool hovered = bounds.contains(Point(lx, ly));
    if (hovered != m_hovered) {
        m_hovered = hovered;
        request_redraw();
        return true;
    }
    return false;
}

bool Spinner::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button != MouseButton::Left) return false;

    if (pressed && bounds.contains(Point(lx, ly))) {
        if (m_is_open) {
            close_dropdown();
        } else {
            open_dropdown();
        }
        return true;
    }
    return false;
}

bool Spinner::on_key(const KeyPressEvent& event) {
    if (!event.pressed) return false;

    if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter || event.keysym == XKB_KEY_space || event.keysym == XKB_KEY_Down) {
        if (!m_is_open) {
            open_dropdown();
            return true;
        }
    }
    return false;
}

} // namespace miqu
