#include "miqutoolkit/view/edit_text.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
namespace miqu {

void EditText::set_window(Window* win) {
    View::set_window(win);
    if (win && m_focused && !win->get_focused_view()) {
        win->set_focused_view(this);
    }
}

void EditText::set_focused(bool focus) {
    m_focused = focus;
    if (focus) {
        request_focus();
    } else {
        if (has_focus()) {
            clear_focus();
        }
    }
}

void EditText::on_focus_changed(bool focused) {
    if (m_focused != focused) {
        m_focused = focused;
        request_redraw();
    }
}

void EditText::set_text(std::string text) {
    m_text = std::move(text);
    m_cursor_pos = static_cast<int>(m_text.size());
}

void EditText::clear() {
    m_text.clear();
    m_cursor_pos = 0;
}

Size EditText::measure_size() const {
    int pad_v = (m_padding.top + m_padding.bottom > 0) ? (m_padding.top + m_padding.bottom) : 16;
    return Size(m_bounds.width, 22 + pad_v);
}

void EditText::draw(cairo_t* cr, const Rect& bounds) {
    m_bounds = bounds;
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();

    int draw_x = bounds.x;
    int draw_y = bounds.y;
    int draw_w = bounds.width;
    int draw_h = bounds.height;

    if (draw_w <= 0 || draw_h <= 0) return;

    cairo_save(cr);
    double radius = static_cast<double>(std::max(0, config->metrics.corner_radius));

    if (m_draw_background) {
        // Background input pill / card
        CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, radius);
        Color bg_col = m_has_custom_bg_color ? m_bg_color : config->colors.surface_variant;
        cairo_set_source_rgba(cr, bg_col.r,
                                  bg_col.g,
                                  bg_col.b,
                                  bg_col.a);
        cairo_fill(cr);

        // Focused outline
        if (m_focused) {
            CardView::draw_rounded_rect(cr, draw_x + 0.5, draw_y + 0.5, draw_w - 1.0, draw_h - 1.0, radius);
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      0.9f);
            cairo_set_line_width(cr, 1.5);
            cairo_stroke(cr);
        }
    }

    // Text content or placeholder
    bool is_hint = m_text.empty();
    std::string display_text;
    int pango_cursor_index = m_cursor_pos;

    if (is_hint) {
        display_text = m_hint;
    } else if (m_password_mode) {
        display_text = "";
        for (size_t i = 0; i < m_text.size(); ++i) {
            display_text += "\xe2\x80\xa2"; // UTF-8 for bullet • (U+2022)
        }
        pango_cursor_index = m_cursor_pos * 3;
    } else {
        display_text = m_text;
    }

    PangoLayout* layout = pango_cairo_create_layout(cr);
    pango_layout_set_text(layout, display_text.c_str(), -1);

    std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
    int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 11;
    std::string font_desc_str = font_family + " " + std::to_string(font_size);
    PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
    pango_layout_set_font_description(layout, desc);
    pango_font_description_free(desc);

    int pad_left = m_padding.left > 0 ? m_padding.left : 14;
    int pad_right = m_padding.right > 0 ? m_padding.right : 14;
    int pad_top = m_padding.top;
    int pad_bottom = m_padding.bottom;
    int avail_text_w = std::max(0, draw_w - pad_left - pad_right);
    int avail_text_h = std::max(0, draw_h - pad_top - pad_bottom);

    pango_layout_set_width(layout, avail_text_w * PANGO_SCALE);
    pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

    int text_w = 0, text_h = 0;
    pango_layout_get_pixel_size(layout, &text_w, &text_h);

    double text_draw_x = draw_x + pad_left;
    double text_draw_y = draw_y + pad_top + (avail_text_h - text_h) / 2.0;

    cairo_move_to(cr, text_draw_x, text_draw_y);

    if (is_hint) {
        if (m_has_custom_hint_color) {
            cairo_set_source_rgba(cr, m_hint_color.r,
                                      m_hint_color.g,
                                      m_hint_color.b,
                                      m_hint_color.a);
        } else {
            cairo_set_source_rgba(cr, config->colors.on_surface_variant.r,
                                      config->colors.on_surface_variant.g,
                                      config->colors.on_surface_variant.b,
                                      0.6f);
        }
    } else {
        if (m_has_custom_text_color) {
            cairo_set_source_rgba(cr, m_text_color.r,
                                      m_text_color.g,
                                      m_text_color.b,
                                      m_text_color.a);
        } else {
            cairo_set_source_rgba(cr, config->colors.on_surface.r,
                                      config->colors.on_surface.g,
                                      config->colors.on_surface.b,
                                      config->colors.on_surface.a);
        }
    }

    pango_cairo_show_layout(cr, layout);

    // Draw Cursor
    if (m_focused && !is_hint) {
        PangoRectangle strong_pos;
        pango_layout_get_cursor_pos(layout, pango_cursor_index, &strong_pos, nullptr);

        double cur_x = text_draw_x + static_cast<double>(strong_pos.x) / PANGO_SCALE;
        double cur_y = text_draw_y + static_cast<double>(strong_pos.y) / PANGO_SCALE;
        double cur_h = static_cast<double>(strong_pos.height) / PANGO_SCALE;

        cairo_set_source_rgba(cr, config->colors.primary.r,
                                  config->colors.primary.g,
                                  config->colors.primary.b,
                                  1.0f);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, cur_x, cur_y);
        cairo_line_to(cr, cur_x, cur_y + cur_h);
        cairo_stroke(cr);
    } else if (m_focused && is_hint) {
        cairo_set_source_rgba(cr, config->colors.primary.r,
                                  config->colors.primary.g,
                                  config->colors.primary.b,
                                  1.0f);
        cairo_set_line_width(cr, 2.0);
        cairo_move_to(cr, text_draw_x, text_draw_y);
        cairo_line_to(cr, text_draw_x, text_draw_y + text_h);
        cairo_stroke(cr);
    }

    g_object_unref(layout);
    cairo_restore(cr);
}

bool EditText::on_key(const KeyPressEvent& event) {
    if (!m_focused || !event.pressed) return false;

    if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter) {
        if (m_on_submit) {
            m_on_submit(m_text);
            return true;
        }
        return false;
    }

    if (event.keysym == XKB_KEY_BackSpace) {
        if (!m_text.empty() && m_cursor_pos > 0) {
            m_text.erase(m_cursor_pos - 1, 1);
            m_cursor_pos--;
            if (m_on_text_changed) m_on_text_changed(std::static_pointer_cast<EditText>(shared_from_this()), m_text);
            return true;
        }
        return false;
    }

    if (event.keysym == XKB_KEY_Delete) {
        if (m_cursor_pos < static_cast<int>(m_text.size())) {
            m_text.erase(m_cursor_pos, 1);
            if (m_on_text_changed) m_on_text_changed(std::static_pointer_cast<EditText>(shared_from_this()), m_text);
            return true;
        }
        return false;
    }

    if (event.keysym == XKB_KEY_Left) {
        if (m_cursor_pos > 0) {
            m_cursor_pos--;
            return true;
        }
        return false;
    }

    if (event.keysym == XKB_KEY_Right) {
        if (m_cursor_pos < static_cast<int>(m_text.size())) {
            m_cursor_pos++;
            return true;
        }
        return false;
    }

    if (event.keysym == XKB_KEY_Home) {
        m_cursor_pos = 0;
        return true;
    }

    if (event.keysym == XKB_KEY_End) {
        m_cursor_pos = static_cast<int>(m_text.size());
        return true;
    }

    if (event.has_ctrl() && (event.keysym == XKB_KEY_u || event.keysym == XKB_KEY_U)) {
        clear();
        if (m_on_text_changed) m_on_text_changed(std::static_pointer_cast<EditText>(shared_from_this()), m_text);
        return true;
    }

    if (event.has_ctrl() && (event.keysym == XKB_KEY_w || event.keysym == XKB_KEY_W)) {
        while (!m_text.empty() && m_cursor_pos > 0 && m_text[m_cursor_pos - 1] == ' ') {
            m_text.erase(m_cursor_pos - 1, 1);
            m_cursor_pos--;
        }
        while (!m_text.empty() && m_cursor_pos > 0 && m_text[m_cursor_pos - 1] != ' ') {
            m_text.erase(m_cursor_pos - 1, 1);
            m_cursor_pos--;
        }
        if (m_on_text_changed) m_on_text_changed(std::static_pointer_cast<EditText>(shared_from_this()), m_text);
        return true;
    }

    if (!event.utf8_text.empty() && event.utf8_text[0] >= 32 && event.utf8_text[0] != 127 && !event.has_ctrl() && !event.has_alt()) {
        m_text.insert(m_cursor_pos, event.utf8_text);
        m_cursor_pos += static_cast<int>(event.utf8_text.size());
        if (m_on_text_changed) m_on_text_changed(std::static_pointer_cast<EditText>(shared_from_this()), m_text);
        return true;
    }

    return false;
}

bool EditText::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button == MouseButton::Left && pressed) {
        int draw_x = bounds.x;
        int draw_y = bounds.y;
        int draw_w = bounds.width;
        int draw_h = bounds.height;
        Rect pill_rect(draw_x, draw_y, draw_w, draw_h);

        if (pill_rect.contains(lx, ly)) {
            request_focus();
            int pad_left = m_padding.left > 0 ? m_padding.left : 14;
            int rel_x = lx - (draw_x + pad_left);

            if (rel_x <= 0) {
                m_cursor_pos = 0;
            } else if (!m_text.empty()) {
                cairo_surface_t* temp_surf = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 1, 1);
                cairo_t* cr = cairo_create(temp_surf);
                PangoLayout* layout = pango_cairo_create_layout(cr);
                pango_layout_set_text(layout, m_text.c_str(), -1);

                auto config = Config::get();
                std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
                int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 11;
                std::string font_desc_str = font_family + " " + std::to_string(font_size);
                PangoFontDescription* desc = pango_font_description_from_string(font_desc_str.c_str());
                pango_layout_set_font_description(layout, desc);
                pango_font_description_free(desc);

                int trailing = 0;
                pango_layout_xy_to_index(layout, rel_x * PANGO_SCALE, 0, &m_cursor_pos, &trailing);
                if (trailing > 0) m_cursor_pos += trailing;
                m_cursor_pos = std::clamp(m_cursor_pos, 0, static_cast<int>(m_text.size()));

                g_object_unref(layout);
                cairo_destroy(cr);
                cairo_surface_destroy(temp_surf);
            }
            if (m_window) m_window->schedule_redraw();
            return true;
        }
    }
    return false;
}

} // namespace miqu
