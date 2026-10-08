#include "miqutoolkit/view/search_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>

namespace miqu {

SearchView::SearchView() {
    m_title_view = TextViewBuilder::create()->bold(true)->textSize(12)->build();
    m_edit_text = EditTextBuilder::create()->focused(true)->padding(0, 0)->drawBackground(false)->build();

    m_edit_text->set_on_text_changed_listener([this](auto, const std::string& text) {
        if (m_on_text_change) {
            m_on_text_change(text);
        }
    });

    add_view(m_title_view);
    add_view(m_edit_text);
}

void SearchView::set_title(std::string title) {
    m_title = std::move(title);
    if (m_title_view) {
        m_title_view->set_text(m_title);
    }
}

void SearchView::set_hint(std::string hint) {
    if (m_edit_text) {
        m_edit_text->set_hint(std::move(hint));
    }
}

const std::string& SearchView::get_hint() const {
    static const std::string empty;
    return m_edit_text ? m_edit_text->get_hint() : empty;
}

void SearchView::set_query(std::string query) {
    if (m_edit_text) {
        m_edit_text->set_text(std::move(query));
    }
}

const std::string& SearchView::get_query() const {
    static const std::string empty;
    return m_edit_text ? m_edit_text->get_text() : empty;
}

void SearchView::set_focused(bool focus) {
    if (m_edit_text) {
        m_edit_text->set_focused(focus);
    }
}

bool SearchView::is_focused() const {
    return m_edit_text ? m_edit_text->is_focused() : false;
}

void SearchView::set_on_query_text_listener(std::function<void(const std::string&)> on_change,
                                            std::function<void(const std::string&)> on_submit) {
    m_on_text_change = std::move(on_change);
    m_on_text_submit = std::move(on_submit);
}

Size SearchView::measure_size() const {
    int pad_h = m_padding.left + m_padding.right;
    int pad_v = m_padding.top + m_padding.bottom;
    return Size(m_bounds.width > 0 ? m_bounds.width : 200 + pad_h, 24 + pad_v);
}

void SearchView::draw(cairo_t* cr, const Rect& bounds) {
    m_bounds = bounds;
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    auto config = Config::get();
    m_child_entries.clear();

    // 1. Calculate Content Bounds
    int draw_x = bounds.x;
    int draw_y = bounds.y;
    int draw_w = bounds.width;
    int draw_h = bounds.height;

    if (draw_w <= 0 || draw_h <= 0) return;

    int radius = (m_corner_radius >= 0) ? m_corner_radius : config->metrics.corner_radius;

    cairo_save(cr);

    // 2. Draw Unified SearchView Pill Surface
    CardView::draw_rounded_rect(cr, draw_x, draw_y, draw_w, draw_h, radius);
    if (m_has_custom_bg) {
        cairo_set_source_rgba(cr, m_bg_color.r, m_bg_color.g, m_bg_color.b, m_bg_color.a);
    } else {
        cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                  config->colors.surface_variant.g,
                                  config->colors.surface_variant.b,
                                  config->colors.surface_variant.a);
    }
    cairo_fill(cr);

    // 3. Focused Outline
    if (is_focused()) {
        CardView::draw_rounded_rect(cr, draw_x + 0.5, draw_y + 0.5, draw_w - 1.0, draw_h - 1.0, radius);
        cairo_set_source_rgba(cr, config->colors.primary.r,
                                  config->colors.primary.g,
                                  config->colors.primary.b,
                                  0.9f);
        cairo_set_line_width(cr, 1.5);
        cairo_stroke(cr);
    } else if (m_stroke_width > 0 && m_stroke_color.a > 0.0f) {
        double offset = m_stroke_width / 2.0;
        CardView::draw_rounded_rect(cr, draw_x + offset, draw_y + offset, draw_w - m_stroke_width, draw_h - m_stroke_width, std::max(0.0, radius - offset));
        cairo_set_source_rgba(cr, m_stroke_color.r, m_stroke_color.g, m_stroke_color.b, m_stroke_color.a);
        cairo_set_line_width(cr, m_stroke_width);
        cairo_stroke(cr);
    }

    cairo_restore(cr);

    // 4. Inset by Padding
    int pad_l = m_padding.left > 0 ? m_padding.left : 16;
    int pad_r = m_padding.right > 0 ? m_padding.right : 16;
    int pad_t = m_padding.top;
    int pad_b = m_padding.bottom;

    int content_x = draw_x + pad_l;
    int content_y = draw_y + pad_t;
    int content_w = std::max(0, draw_w - pad_l - pad_r);
    int content_h = std::max(0, draw_h - pad_t - pad_b);

    if (content_w <= 0 || content_h <= 0) return;

    int current_x = content_x;

    // 5. Draw Left Title/Label if present
    if (!m_title.empty() && m_title_view) {
        m_title_view->set_text_color(config->colors.primary);
        Size title_size = m_title_view->measure_size();
        int title_w = title_size.width;
        int title_h = title_size.height;
        int title_y = content_y + (content_h - title_h) / 2;

        Rect title_rect(current_x, title_y, title_w, title_h);
        m_child_entries.push_back({m_title_view, title_rect});
        m_title_view->draw(cr, title_rect);

        current_x += title_w + 12; // Gap between title and input field
    }

    // 6. Draw Inner EditText spanning the rest of the right side
    if (m_edit_text) {
        int text_w = std::max(0, content_x + content_w - current_x);
        Rect text_rect(current_x, content_y, text_w, content_h);

        m_child_entries.push_back({m_edit_text, text_rect});
        m_edit_text->draw(cr, text_rect);
    }
}

bool SearchView::on_key(const KeyPressEvent& event) {
    if (!is_visible() || !event.pressed) return false;

    if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter) {
        if (m_on_text_submit && m_edit_text) {
            m_on_text_submit(m_edit_text->get_text());
            return true;
        }
    }

    if (m_edit_text) {
        return m_edit_text->on_key(event);
    }
    return false;
}

bool SearchView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible()) return false;

    int draw_x = bounds.x;
    int draw_y = bounds.y;
    int draw_w = bounds.width;
    int draw_h = bounds.height;
    Rect pill_rect(draw_x, draw_y, draw_w, draw_h);

    if (button == MouseButton::Left && pressed) {
        if (pill_rect.contains(lx, ly)) {
            set_focused(true);
            if (m_edit_text) {
                for (const auto& entry : m_child_entries) {
                    if (entry.view == m_edit_text) {
                        return m_edit_text->on_mouse_button(lx, ly, button, pressed, entry.allocated_bounds);
                    }
                }
            }
            return true;
        } else {
            set_focused(false);
        }
    }
    return false;
}

bool SearchView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    return ViewGroup::on_mouse_move(lx, ly, bounds);
}

} // namespace miqu
