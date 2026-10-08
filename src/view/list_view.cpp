#include "miqutoolkit/view/list_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
#include <cmath>

namespace miqu {

ListView::ListView() {}

void ListView::set_items(std::vector<std::shared_ptr<View>> items) {
    m_items = std::move(items);
    m_selected_index = m_items.empty() ? -1 : 0;
    m_scroll_y = 0.0;
}

void ListView::add_item(std::shared_ptr<View> item) {
    if (item) {
        m_items.push_back(std::move(item));
        if (m_selected_index < 0 && !m_items.empty()) {
            m_selected_index = 0;
        }
    }
}

void ListView::clear_items() {
    m_items.clear();
    m_selected_index = -1;
    m_hovered_index = -1;
    m_scroll_y = 0.0;
}

std::shared_ptr<View> ListView::get_item_at(size_t index) const {
    if (index < m_items.size()) {
        return m_items[index];
    }
    return nullptr;
}

void ListView::set_selected_index(int index) {
    if (m_items.empty()) {
        m_selected_index = -1;
        return;
    }
    m_selected_index = std::clamp(index, 0, static_cast<int>(m_items.size()) - 1);
    if (m_last_height > 0) {
        ensure_visible(m_last_height);
    }
}

std::shared_ptr<View> ListView::get_selected_item() const {
    if (m_selected_index >= 0 && m_selected_index < static_cast<int>(m_items.size())) {
        return m_items[m_selected_index];
    }
    return nullptr;
}

void ListView::ensure_visible(int viewport_height) {
    if (m_selected_index < 0 || viewport_height <= 0) return;

    int row_stride = m_item_h + m_spacing;
    double item_top = m_selected_index * row_stride;
    double item_bottom = item_top + m_item_h;

    if (item_top < m_scroll_y) {
        m_scroll_y = item_top;
    } else if (item_bottom > m_scroll_y + viewport_height) {
        m_scroll_y = item_bottom - viewport_height;
    }

    double content_h = m_items.size() * row_stride - m_spacing;
    double max_scroll = std::max(0.0, content_h - viewport_height);
    m_scroll_y = std::clamp(m_scroll_y, 0.0, max_scroll);
}

int ListView::item_at(int lx, int ly, const Rect& bounds) const {
    int rel_x = lx - bounds.x;
    int rel_y = ly - bounds.y + static_cast<int>(m_scroll_y);

    if (rel_x < 0 || rel_x >= bounds.width || rel_y < 0) {
        return -1;
    }

    int row_stride = m_item_h + m_spacing;
    if (row_stride <= 0) return -1;

    int idx = rel_y / row_stride;
    int offset_in_row = rel_y % row_stride;

    if (offset_in_row > m_item_h) {
        return -1; // Spacing gap
    }

    if (idx >= 0 && idx < static_cast<int>(m_items.size())) {
        return idx;
    }
    return -1;
}

void ListView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_last_height = bounds.height;
    auto config = Config::get();

    int row_stride = m_item_h + m_spacing;
    double content_h = std::max(0, static_cast<int>(m_items.size()) * row_stride - m_spacing);
    double max_scroll = std::max(0.0, content_h - bounds.height);

    m_scroll_y = std::clamp(m_scroll_y, 0.0, max_scroll);

    cairo_save(cr);
    cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
    cairo_clip(cr);

    int start_idx = std::max(0, static_cast<int>(m_scroll_y / row_stride));
    int end_idx = std::min(static_cast<int>(m_items.size()), static_cast<int>((m_scroll_y + bounds.height) / row_stride) + 1);

    double radius = m_corner_radius > 0 ? static_cast<double>(m_corner_radius) : 6.0;

    for (int i = start_idx; i < end_idx; ++i) {
        int item_y = bounds.y + i * row_stride - static_cast<int>(m_scroll_y);
        Rect item_rect(bounds.x, item_y, bounds.width, m_item_h);

        bool is_selected = (i == m_selected_index);
        bool is_hovered = (i == m_hovered_index && !is_selected);

        // Highlight background
        if (is_selected) {
            CardView::draw_rounded_rect(cr, item_rect.x, item_rect.y, item_rect.width, item_rect.height, radius);
            cairo_set_source_rgba(cr, config->colors.primary_container.r,
                                      config->colors.primary_container.g,
                                      config->colors.primary_container.b,
                                      0.75f);
            cairo_fill(cr);

            CardView::draw_rounded_rect(cr, item_rect.x + 0.5, item_rect.y + 0.5, item_rect.width - 1.0, item_rect.height - 1.0, radius);
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      0.45f);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else if (is_hovered) {
            CardView::draw_rounded_rect(cr, item_rect.x, item_rect.y, item_rect.width, item_rect.height, radius);
            cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                      config->colors.surface_variant.g,
                                      config->colors.surface_variant.b,
                                      0.45f);
            cairo_fill(cr);
        }

        // Draw child item
        if (m_items[i]) {
            m_items[i]->draw(cr, item_rect);
        }

        // Optional row divider
        if (m_show_dividers && i < static_cast<int>(m_items.size()) - 1) {
            int div_y = item_y + m_item_h + m_spacing / 2;
            cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                      config->colors.outline_variant.g,
                                      config->colors.outline_variant.b,
                                      0.3f);
            cairo_set_line_width(cr, 1.0);
            cairo_move_to(cr, bounds.x + 12, div_y);
            cairo_line_to(cr, bounds.x + bounds.width - 12, div_y);
            cairo_stroke(cr);
        }
    }

    // Scrollbar (if content overflows)
    if (content_h > bounds.height) {
        double scrollbar_w = 4.0;
        double scrollbar_x = bounds.x + bounds.width - scrollbar_w - 2.0;
        double visible_ratio = static_cast<double>(bounds.height) / content_h;
        double thumb_h = std::max(20.0, bounds.height * visible_ratio);
        double thumb_y = bounds.y + (m_scroll_y / max_scroll) * (bounds.height - thumb_h);

        cairo_set_source_rgba(cr, config->colors.on_surface.r,
                                  config->colors.on_surface.g,
                                  config->colors.on_surface.b,
                                  0.25f);
        double sr = scrollbar_w / 2.0;
        cairo_new_sub_path(cr);
        cairo_arc(cr, scrollbar_x + scrollbar_w - sr, thumb_y + sr, sr, -M_PI / 2, 0);
        cairo_arc(cr, scrollbar_x + scrollbar_w - sr, thumb_y + thumb_h - sr, sr, 0, M_PI / 2);
        cairo_arc(cr, scrollbar_x + sr, thumb_y + thumb_h - sr, sr, M_PI / 2, M_PI);
        cairo_arc(cr, scrollbar_x + sr, thumb_y + sr, sr, M_PI, 3 * M_PI / 2);
        cairo_close_path(cr);
        cairo_fill(cr);
    }

    cairo_restore(cr);
}

bool ListView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    int old_hover = m_hovered_index;
    m_hovered_index = item_at(lx, ly, bounds);
    return m_hovered_index != old_hover;
}

bool ListView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button == MouseButton::Left && pressed) {
        int idx = item_at(lx, ly, bounds);
        if (idx >= 0 && idx < static_cast<int>(m_items.size())) {
            m_selected_index = idx;
            if (m_on_item_click) {
                m_on_item_click(idx, m_items[idx]);
            }
            return true;
        }
    }
    return false;
}

bool ListView::on_scroll(double delta) {
    if (m_items.empty() || m_last_height <= 0) return false;

    int row_stride = m_item_h + m_spacing;
    double content_h = std::max(0, static_cast<int>(m_items.size()) * row_stride - m_spacing);
    double max_scroll = std::max(0.0, content_h - m_last_height);

    if (max_scroll <= 0.0) return false;

    double old_scroll = m_scroll_y;
    m_scroll_y = std::clamp(m_scroll_y + delta * 30.0, 0.0, max_scroll);
    return std::abs(m_scroll_y - old_scroll) > 0.01;
}

bool ListView::on_key(const KeyPressEvent& event) {
    if (m_items.empty()) return false;

    int count = static_cast<int>(m_items.size());
    int old_selected = m_selected_index;

    switch (event.keysym) {
        case XKB_KEY_Up:
        case XKB_KEY_k:
            if (m_selected_index > 0) {
                m_selected_index--;
            }
            break;

        case XKB_KEY_Down:
        case XKB_KEY_j:
            if (m_selected_index < count - 1) {
                m_selected_index++;
            }
            break;

        case XKB_KEY_Page_Up: {
            int page_items = std::max(1, m_last_height / (m_item_h + m_spacing));
            m_selected_index = std::max(0, m_selected_index - page_items);
            break;
        }

        case XKB_KEY_Page_Down: {
            int page_items = std::max(1, m_last_height / (m_item_h + m_spacing));
            m_selected_index = std::min(count - 1, m_selected_index + page_items);
            break;
        }

        case XKB_KEY_Home:
            m_selected_index = 0;
            break;

        case XKB_KEY_End:
            m_selected_index = count - 1;
            break;

        case XKB_KEY_Return:
        case XKB_KEY_KP_Enter:
            if (m_selected_index >= 0 && m_selected_index < count) {
                if (m_on_item_click) {
                    m_on_item_click(m_selected_index, m_items[m_selected_index]);
                }
                return true;
            }
            break;

        default:
            return false;
    }

    if (m_selected_index != old_selected) {
        if (m_last_height > 0) {
            ensure_visible(m_last_height);
        }
        return true;
    }

    return false;
}

bool ListView::on_touch(const TouchEvent& event, const Rect& bounds) {
    if (!is_visible()) return false;

    if (event.phase == TouchPhase::Down && !bounds.contains(Point(static_cast<int>(event.x), static_cast<int>(event.y)))) {
        return false;
    }

    if (event.phase == TouchPhase::Down) {
        m_touch_id = event.id;
        m_touch_start_x = event.x;
        m_touch_start_y = event.y;
        m_touch_last_y = event.y;
        m_touch_scrolling = false;
        m_hovered_index = item_at(static_cast<int>(event.x), static_cast<int>(event.y), bounds);
        if (get_window()) get_window()->schedule_redraw();
        return true;
    }

    if (event.id != m_touch_id) return false;

    if (event.phase == TouchPhase::Motion) {
        double dy = event.y - m_touch_last_y;
        m_touch_last_y = event.y;

        if (!m_touch_scrolling) {
            double total_dy = std::abs(event.y - m_touch_start_y);
            double total_dx = std::abs(event.x - m_touch_start_x);
            if (total_dy > 8.0 && total_dy > total_dx) {
                m_touch_scrolling = true;
                m_hovered_index = -1;
            }
        }

        if (m_touch_scrolling) {
            int row_stride = m_item_h + m_spacing;
            double content_h = std::max(0, static_cast<int>(m_items.size()) * row_stride - m_spacing);
            double max_scroll = std::max(0.0, content_h - m_last_height);
            if (max_scroll > 0.0) {
                double old_scroll = m_scroll_y;
                m_scroll_y = std::clamp(m_scroll_y - dy, 0.0, max_scroll);
                if (m_scroll_y != old_scroll && get_window()) {
                    get_window()->schedule_redraw();
                }
            }
            return true;
        }
        return true;
    }

    if (event.phase == TouchPhase::Up) {
        m_touch_id = -1;
        if (m_touch_scrolling) {
            m_touch_scrolling = false;
            m_hovered_index = -1;
            if (get_window()) get_window()->schedule_redraw();
            return true;
        }

        m_touch_scrolling = false;
        int idx = item_at(static_cast<int>(event.x), static_cast<int>(event.y), bounds);
        if (idx >= 0 && idx < static_cast<int>(m_items.size())) {
            m_selected_index = idx;
            m_hovered_index = idx;
            if (m_on_item_click) {
                m_on_item_click(idx, m_items[idx]);
            }
            if (get_window()) get_window()->schedule_redraw();
            return true;
        }
        return true;
    }

    if (event.phase == TouchPhase::Cancel) {
        m_touch_id = -1;
        m_touch_scrolling = false;
        m_hovered_index = -1;
        if (get_window()) get_window()->schedule_redraw();
        return true;
    }

    return false;
}

} // namespace miqu

