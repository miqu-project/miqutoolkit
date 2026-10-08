#include "miqutoolkit/view/scroll_view.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <algorithm>

namespace miqu {

void ScrollView::set_content_view(std::shared_ptr<View> view) {
    clear_views();
    m_content = std::move(view);
    if (m_content) {
        add_view(m_content);
    }
}

void ScrollView::set_scroll_y(double y) {
    m_scroll_y = std::clamp(y, 0.0, m_max_scroll);
    if (m_window) m_window->schedule_redraw();
}

Size ScrollView::measure_size() const {
    return measure_size(-1);
}

Size ScrollView::measure_size(int avail_width) const {
    if (!m_content || m_content->get_visibility() == Visibility::Gone) {
        return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);
    }
    const auto& margin = m_content->get_margin();
    int content_avail_w = (avail_width >= 0) ?
        std::max(0, avail_width - m_padding.left - m_padding.right - margin.left - margin.right) : -1;
    Size child_size = m_content->measure_size(content_avail_w);
    int w = child_size.width + margin.left + margin.right + m_padding.left + m_padding.right;
    int h = child_size.height + margin.top + margin.bottom + m_padding.top + m_padding.bottom;
    return Size(w, h);
}

void ScrollView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    draw_background(cr, bounds);

    Rect content_rect = get_content_rect(bounds);
    if (content_rect.width <= 0 || content_rect.height <= 0 || !m_content || m_content->get_visibility() == Visibility::Gone) {
        draw_stroke(cr, bounds);
        return;
    }

    const auto& params = m_content->get_layout_params();
    const auto& margin = m_content->get_margin();
    int content_avail_w = std::max(0, content_rect.width - margin.left - margin.right);
    Size measured = m_content->measure_size(content_avail_w);

    int child_w = (params.width >= 0) ? params.width : content_avail_w;
    int child_h = (params.height >= 0) ? params.height : measured.height;
    if (params.height == static_cast<int>(LayoutDimension::MatchParent) && child_h < content_rect.height - margin.top - margin.bottom) {
        child_h = content_rect.height - margin.top - margin.bottom;
    }

    int total_child_h = child_h + margin.top + margin.bottom;
    m_max_scroll = std::max(0.0, static_cast<double>(total_child_h - content_rect.height));
    m_scroll_y = std::clamp(m_scroll_y, 0.0, m_max_scroll);

    // Clip to content rect
    cairo_save(cr);
    cairo_rectangle(cr, content_rect.x, content_rect.y, content_rect.width, content_rect.height);
    cairo_clip(cr);

    int child_x = content_rect.x + margin.left;
    int child_y = content_rect.y + margin.top - static_cast<int>(m_scroll_y);
    Rect child_bounds(child_x, child_y, child_w, child_h);

    m_child_entries.clear();
    m_content->set_bounds(child_bounds);
    m_child_entries.push_back({m_content, child_bounds});
    m_content->draw(cr, child_bounds);

    cairo_restore(cr);

    // Draw overlay scrollbar thumb if content overflows
    if (m_show_scrollbar && m_max_scroll > 0.0) {
        int bar_w = 4;
        int bar_margin = 3;
        int track_h = content_rect.height - 8;
        int thumb_h = std::max(20, static_cast<int>((static_cast<double>(content_rect.height) / total_child_h) * track_h));
        int thumb_y = content_rect.y + 4 + static_cast<int>((m_scroll_y / m_max_scroll) * (track_h - thumb_h));
        int thumb_x = content_rect.x + content_rect.width - bar_w - bar_margin;

        cairo_save(cr);
        CardView::draw_rounded_rect(cr, thumb_x, thumb_y, bar_w, thumb_h, bar_w / 2.0);
        auto config = Config::get();
        float alpha = (m_dragging_thumb || m_is_hovered) ? 0.45f : 0.20f;
        cairo_set_source_rgba(cr, config->colors.on_surface.r, config->colors.on_surface.g, config->colors.on_surface.b, alpha);
        cairo_fill(cr);
        cairo_restore(cr);
    }

    draw_stroke(cr, bounds);
}

bool ScrollView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    if (!is_visible()) return false;

    m_is_hovered = bounds.contains(Point(lx, ly));

    if (m_dragging_thumb && m_max_scroll > 0.0) {
        int content_h = get_content_rect(bounds).height;
        int track_h = content_h - 8;
        Size measured = m_content ? m_content->measure_size() : Size(0, 0);
        int total_child_h = measured.height;
        int thumb_h = std::max(20, static_cast<int>((static_cast<double>(content_h) / total_child_h) * track_h));
        int available_track = track_h - thumb_h;
        if (available_track > 0) {
            double dy = ly - m_drag_start_y;
            double target_scroll = m_drag_start_scroll + (dy / available_track) * m_max_scroll;
            m_scroll_y = std::clamp(target_scroll, 0.0, m_max_scroll);
            if (m_window) m_window->schedule_redraw();
        }
        return true;
    }

    // Forward to child view if mouse is within visible content rect
    Rect content_rect = get_content_rect(bounds);
    if (content_rect.contains(Point(lx, ly)) && m_content && m_content->is_visible()) {
        if (!m_child_entries.empty()) {
            m_content->on_mouse_move(lx, ly, m_child_entries[0].allocated_bounds);
        }
    }
    return m_is_hovered;
}

bool ScrollView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible()) return false;

    Rect content_rect = get_content_rect(bounds);
    if (!content_rect.contains(Point(lx, ly))) return false;

    // Check if clicking on the scrollbar
    if (button == MouseButton::Left && m_max_scroll > 0.0) {
        int bar_w = 10; // hit area
        int scrollbar_x = content_rect.x + content_rect.width - bar_w;
        if (lx >= scrollbar_x) {
            if (pressed) {
                m_dragging_thumb = true;
                m_drag_start_y = ly;
                m_drag_start_scroll = m_scroll_y;
                return true;
            } else if (m_dragging_thumb) {
                m_dragging_thumb = false;
                return true;
            }
        }
    }

    if (!pressed && m_dragging_thumb) {
        m_dragging_thumb = false;
        return true;
    }

    // Forward to child view
    if (m_content && m_content->is_visible() && !m_child_entries.empty()) {
        return m_content->on_mouse_button(lx, ly, button, pressed, m_child_entries[0].allocated_bounds);
    }

    return false;
}

bool ScrollView::on_scroll(double delta) {
    if (!is_visible() || m_max_scroll <= 0.0) return false;

    double old_scroll = m_scroll_y;
    m_scroll_y = std::clamp(m_scroll_y + delta * 30.0, 0.0, m_max_scroll);

    if (m_scroll_y != old_scroll) {
        if (m_window) m_window->schedule_redraw();
        return true;
    }
    return false;
}

bool ScrollView::on_touch(const TouchEvent& event, const Rect& bounds) {
    if (!is_visible()) return false;

    Rect content_rect = get_content_rect(bounds);

    if (event.phase == TouchPhase::Down) {
        if (!content_rect.contains(Point(static_cast<int>(event.x), static_cast<int>(event.y)))) {
            return false;
        }

        m_touch_id = event.id;
        m_touch_start_x = event.x;
        m_touch_start_y = event.y;
        m_touch_last_y = event.y;
        m_touch_scrolling = false;
        m_child_touch_target = false;

        if (m_content && m_content->is_visible() && !m_child_entries.empty()) {
            m_child_touch_target = m_content->on_touch(event, m_child_entries[0].allocated_bounds);
        }
        return true;
    }

    if (event.id != m_touch_id) {
        return false;
    }

    if (event.phase == TouchPhase::Motion) {
        double dy = event.y - m_touch_last_y;
        m_touch_last_y = event.y;

        if (!m_touch_scrolling) {
            double total_dy = std::abs(event.y - m_touch_start_y);
            double total_dx = std::abs(event.x - m_touch_start_x);
            if (total_dy > 10.0 && total_dy > total_dx && m_max_scroll > 0.0) {
                m_touch_scrolling = true;
                if (m_child_touch_target && m_content && m_content->is_visible() && !m_child_entries.empty()) {
                    TouchEvent cancel_event = event;
                    cancel_event.phase = TouchPhase::Cancel;
                    m_content->on_touch(cancel_event, m_child_entries[0].allocated_bounds);
                    m_child_touch_target = false;
                }
            } else if (m_child_touch_target && m_content && m_content->is_visible() && !m_child_entries.empty()) {
                m_content->on_touch(event, m_child_entries[0].allocated_bounds);
                return true;
            }
        }

        if (m_touch_scrolling && m_max_scroll > 0.0) {
            double old_scroll = m_scroll_y;
            m_scroll_y = std::clamp(m_scroll_y - dy, 0.0, m_max_scroll);
            if (m_scroll_y != old_scroll && m_window) {
                m_window->schedule_redraw();
            }
            return true;
        }
        return true;
    }

    if (event.phase == TouchPhase::Up) {
        m_touch_id = -1;
        if (m_touch_scrolling) {
            m_touch_scrolling = false;
            m_child_touch_target = false;
            return true;
        }

        m_touch_scrolling = false;
        if (m_child_touch_target && m_content && m_content->is_visible() && !m_child_entries.empty()) {
            m_content->on_touch(event, m_child_entries[0].allocated_bounds);
            m_child_touch_target = false;
        }
        return true;
    }

    if (event.phase == TouchPhase::Cancel) {
        m_touch_id = -1;
        m_touch_scrolling = false;
        if (m_child_touch_target && m_content && m_content->is_visible() && !m_child_entries.empty()) {
            m_content->on_touch(event, m_child_entries[0].allocated_bounds);
            m_child_touch_target = false;
        }
        return true;
    }

    return false;
}

} // namespace miqu

