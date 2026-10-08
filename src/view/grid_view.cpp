#include "miqutoolkit/view/grid_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
#include <iostream>
#include <cmath>

namespace miqu {

GridView::GridView() {
}

void GridView::set_items(std::vector<std::shared_ptr<View>> items) {
    m_items = std::move(items);
    m_item_provider = nullptr;
    m_virtual_count = 0;
    m_selected_index = m_items.empty() ? -1 : 0;
    m_scroll_y = 0.0;
}

void GridView::add_item(std::shared_ptr<View> item) {
    if (item) {
        m_items.push_back(std::move(item));
        if (m_selected_index < 0 && !m_items.empty()) {
            m_selected_index = 0;
        }
    }
}

void GridView::set_item_provider(size_t total_count, ItemProviderCallback provider) {
    m_items.clear();
    m_virtual_count = total_count;
    m_item_provider = std::move(provider);
    m_selected_index = (total_count == 0) ? -1 : 0;
    m_scroll_y = 0.0;
}

void GridView::clear_items() {
    m_items.clear();
    m_virtual_count = 0;
    m_item_provider = nullptr;
    m_selected_index = -1;
    m_hovered_index = -1;
    m_scroll_y = 0.0;
}

std::shared_ptr<View> GridView::get_item_at(size_t index) const {
    if (m_item_provider && index < m_virtual_count) {
        return m_item_provider(index);
    }
    if (index < m_items.size()) {
        return m_items[index];
    }
    return nullptr;
}

void GridView::set_selected_index(int index) {
    size_t count = get_item_count();
    if (count == 0) {
        m_selected_index = -1;
        return;
    }
    m_selected_index = std::clamp(index, 0, static_cast<int>(count) - 1);
    if (m_last_height > 0 && m_effective_cols > 0) {
        int row_stride = m_cell_h + m_space_y;
        ensure_visible(m_last_height, m_effective_cols, row_stride);
    }
}

std::shared_ptr<View> GridView::get_selected_item() const {
    if (m_selected_index >= 0 && m_selected_index < static_cast<int>(get_item_count())) {
        return get_item_at(static_cast<size_t>(m_selected_index));
    }
    return nullptr;
}

int GridView::compute_columns(int bounds_w, int& out_cell_w) const {
    if (bounds_w <= 0) {
        out_cell_w = m_cell_w;
        return 1;
    }

    int cols = m_cols;
    if (m_auto_fit) {
        cols = std::max(1, (bounds_w + m_space_x) / (m_min_col_w + m_space_x));
    }

    if (m_stretch_mode == StretchMode::ColumnWidth && cols > 0) {
        int total_spacing = (cols - 1) * m_space_x;
        out_cell_w = std::max(20, (bounds_w - total_spacing) / cols);
    } else {
        out_cell_w = m_cell_w;
    }

    m_effective_cols = cols;
    m_effective_cell_w = out_cell_w;
    return cols;
}

void GridView::ensure_visible(int viewport_height, int cols, int row_stride) {
    if (m_selected_index < 0 || viewport_height <= 0 || cols <= 0) return;

    int row = m_selected_index / cols;
    double item_top = row * row_stride;
    double item_bottom = item_top + m_cell_h;

    if (item_top < m_scroll_y) {
        m_scroll_y = item_top;
    } else if (item_bottom > m_scroll_y + viewport_height) {
        m_scroll_y = item_bottom - viewport_height;
    }

    int total_rows = (static_cast<int>(get_item_count()) + cols - 1) / cols;
    double content_h = total_rows * row_stride - m_space_y;
    double max_scroll = std::max(0.0, content_h - viewport_height);
    m_scroll_y = std::clamp(m_scroll_y, 0.0, max_scroll);
}

int GridView::item_at(int lx, int ly, const Rect& bounds) const {
    int cell_w = m_effective_cell_w;
    int cols = compute_columns(bounds.width, cell_w);

    int rel_x = lx - bounds.x;
    int rel_y = ly - bounds.y + static_cast<int>(m_scroll_y);

    if (rel_x < 0 || rel_x >= bounds.width || rel_y < 0) {
        return -1;
    }

    int col = rel_x / (cell_w + m_space_x);
    int row = rel_y / (m_cell_h + m_space_y);

    if (col >= cols) return -1;

    int idx = row * cols + col;
    if (idx >= 0 && idx < static_cast<int>(get_item_count())) {
        return idx;
    }
    return -1;
}

void GridView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    m_last_width = bounds.width;
    m_last_height = bounds.height;
    auto config = Config::get();

    int cell_w = 0;
    int cols = compute_columns(bounds.width, cell_w);

    size_t count = get_item_count();
    int total_rows = (static_cast<int>(count) + cols - 1) / cols;
    int row_stride = m_cell_h + m_space_y;
    double content_h = std::max(0, total_rows * row_stride - m_space_y);
    double max_scroll = std::max(0.0, content_h - bounds.height);

    m_scroll_y = std::clamp(m_scroll_y, 0.0, max_scroll);

    cairo_save(cr);
    cairo_rectangle(cr, bounds.x, bounds.y, bounds.width, bounds.height);
    cairo_clip(cr);

    int start_row = std::max(0, static_cast<int>(m_scroll_y / row_stride));
    int end_row = std::min(total_rows, static_cast<int>((m_scroll_y + bounds.height) / row_stride) + 1);

    int start_idx = start_row * cols;
    int end_idx = std::min(end_row * cols, static_cast<int>(count));

    for (int i = start_idx; i < end_idx; ++i) {
        auto item = get_item_at(static_cast<size_t>(i));
        if (!item) continue;

        int grid_row = i / cols;
        int grid_col = i % cols;

        int cell_x = bounds.x + grid_col * (cell_w + m_space_x);
        double cell_y = bounds.y + grid_row * row_stride - m_scroll_y;
        Rect cell_rect(cell_x, static_cast<int>(cell_y), cell_w, m_cell_h);

        bool is_selected = (i == m_selected_index);
        bool is_hovered = (i == m_hovered_index);

        double cell_radius = static_cast<double>(std::max(0, config->metrics.corner_radius));

        // Tile background (Selected / Hovered highlight)
        if (is_selected) {
            CardView::draw_rounded_rect(cr, cell_x, cell_y, cell_w, m_cell_h, cell_radius);
            cairo_set_source_rgba(cr, config->colors.primary_container.r,
                                      config->colors.primary_container.g,
                                      config->colors.primary_container.b,
                                      0.6f);
            cairo_fill(cr);

            CardView::draw_rounded_rect(cr, cell_x + 0.5, cell_y + 0.5, cell_w - 1.0, m_cell_h - 1.0, cell_radius);
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      0.8f);
            cairo_set_line_width(cr, 1.0);
            cairo_stroke(cr);
        } else if (is_hovered) {
            CardView::draw_rounded_rect(cr, cell_x, cell_y, cell_w, m_cell_h, cell_radius);
            cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                      config->colors.surface_variant.g,
                                      config->colors.surface_variant.b,
                                      0.4f);
            cairo_fill(cr);
        }

        // Delegate drawing the cell content to the child View!
        item->draw(cr, cell_rect);
    }

    // Draw Visual Scrollbar Thumb Indicator if content overflows
    if (max_scroll > 0.0) {
        double track_h = bounds.height;
        double thumb_h = std::max(24.0, (static_cast<double>(bounds.height) / content_h) * track_h);
        double scroll_ratio = m_scroll_y / max_scroll;
        double thumb_y = bounds.y + scroll_ratio * (track_h - thumb_h);
        double thumb_x = bounds.x + bounds.width - 4.0;

        cairo_save(cr);
        CardView::draw_rounded_rect(cr, thumb_x, thumb_y, 4.0, thumb_h, 2.0);
        cairo_set_source_rgba(cr, config->colors.outline.r,
                                  config->colors.outline.g,
                                  config->colors.outline.b,
                                  0.6f);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    cairo_restore(cr);
}

bool GridView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    int old_hover = m_hovered_index;
    m_hovered_index = item_at(lx, ly, bounds);

    if (m_hovered_index != old_hover) {
        if (get_window()) {
            get_window()->schedule_redraw();
        }
        return true;
    }
    return false;
}

bool GridView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (button == MouseButton::Left && pressed) {
        int idx = item_at(lx, ly, bounds);
        if (idx >= 0 && idx < static_cast<int>(get_item_count())) {
            m_selected_index = idx;
            if (m_on_item_click) {
                m_on_item_click(idx, get_item_at(static_cast<size_t>(idx)));
            }
            if (get_window()) {
                get_window()->schedule_redraw();
            }
            return true;
        }
    }
    return false;
}

bool GridView::on_scroll(double delta) {
    int cell_w = 0;
    int cols = compute_columns(m_last_width, cell_w);
    int total_rows = (static_cast<int>(get_item_count()) + cols - 1) / cols;
    int row_stride = m_cell_h + m_space_y;
    double content_h = total_rows * row_stride - m_space_y;
    double max_scroll = std::max(0.0, content_h - m_last_height);

    double old_scroll = m_scroll_y;
    m_scroll_y = std::clamp(m_scroll_y + delta * 30.0, 0.0, max_scroll);

    if (m_scroll_y != old_scroll) {
        if (get_window()) {
            get_window()->schedule_redraw();
        }
        return true;
    }
    return false;
}

bool GridView::on_key(const KeyPressEvent& event) {
    int total = static_cast<int>(get_item_count());
    if (!event.pressed || total == 0) return false;

    int cell_w = 0;
    int cols = compute_columns(m_last_width, cell_w);
    int old_sel = m_selected_index;

    switch (event.keysym) {
        case XKB_KEY_Left:
            if (m_selected_index > 0) m_selected_index--;
            break;
        case XKB_KEY_Right:
            if (m_selected_index < total - 1) m_selected_index++;
            break;
        case XKB_KEY_Up:
            if (m_selected_index - cols >= 0) m_selected_index -= cols;
            break;
        case XKB_KEY_Down:
            if (m_selected_index + cols < total) m_selected_index += cols;
            else if (m_selected_index < total - 1) m_selected_index = total - 1;
            break;
        case XKB_KEY_Home:
            m_selected_index = 0;
            break;
        case XKB_KEY_End:
            m_selected_index = total - 1;
            break;
        case XKB_KEY_Return:
        case XKB_KEY_KP_Enter:
            if (m_selected_index >= 0 && m_selected_index < total) {
                if (m_on_item_click) {
                    m_on_item_click(m_selected_index, get_item_at(static_cast<size_t>(m_selected_index)));
                }
                return true;
            }
            break;
        case XKB_KEY_Tab:
            if (event.modifiers & static_cast<uint32_t>(KeyboardModifier::Shift)) {
                if (m_selected_index > 0) m_selected_index--;
                else m_selected_index = total - 1;
            } else {
                if (m_selected_index < total - 1) m_selected_index++;
                else m_selected_index = 0;
            }
            break;
        default:
            return false;
    }

    if (m_selected_index != old_sel) {
        int row_stride = m_cell_h + m_space_y;
        ensure_visible(m_last_height, cols, row_stride);
        if (get_window()) {
            get_window()->schedule_redraw();
        }
        return true;
    }

    return false;
}

bool GridView::on_touch(const TouchEvent& event, const Rect& bounds) {
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
            int cell_w = 0;
            int cols = compute_columns(m_last_width, cell_w);
            int total_rows = (static_cast<int>(get_item_count()) + cols - 1) / cols;
            int row_stride = m_cell_h + m_space_y;
            double content_h = total_rows * row_stride - m_space_y;
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
        if (idx >= 0 && idx < static_cast<int>(get_item_count())) {
            m_selected_index = idx;
            m_hovered_index = idx;
            if (m_on_item_click) {
                m_on_item_click(idx, get_item_at(static_cast<size_t>(idx)));
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

