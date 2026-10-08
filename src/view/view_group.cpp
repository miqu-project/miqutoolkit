#include "miqutoolkit/view/view_group.hpp"
#include "miqutoolkit/core/window.hpp"
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

void View::request_redraw() {
    if (m_window) {
        m_window->schedule_redraw();
    }
}

void View::request_focus() {
    if (m_window) {
        m_window->set_focused_view(this);
    }
}

void View::clear_focus() {
    if (m_window && m_window->get_focused_view() == this) {
        m_window->clear_focus();
    }
}

bool View::has_focus() const {
    return m_window && (m_window->get_focused_view() == this);
}

void ViewGroup::draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r) {
    if (r <= 0.0) {
        cairo_rectangle(cr, x, y, w, h);
        return;
    }
    double max_r = std::min(w, h) / 2.0;
    if (r > max_r) r = max_r;
    double deg = M_PI / 180.0;
    cairo_new_sub_path(cr);
    cairo_arc(cr, x + w - r, y + r, r, -90 * deg, 0 * deg);
    cairo_arc(cr, x + w - r, y + h - r, r, 0 * deg, 90 * deg);
    cairo_arc(cr, x + r, y + h - r, r, 90 * deg, 180 * deg);
    cairo_arc(cr, x + r, y + r, r, 180 * deg, 270 * deg);
    cairo_close_path(cr);
}

void ViewGroup::draw_background(cairo_t* cr, const Rect& bounds) const {
    if (!m_has_custom_bg || m_background_color.a <= 0.0f) return;

    cairo_save(cr);
    draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, m_corner_radius);
    cairo_set_source_rgba(cr, m_background_color.r, m_background_color.g, m_background_color.b, m_background_color.a);
    cairo_fill(cr);
    cairo_restore(cr);
}

void ViewGroup::draw_stroke(cairo_t* cr, const Rect& bounds) const {
    if (m_stroke_width <= 0 || m_stroke_color.a <= 0.0f) return;

    cairo_save(cr);
    double offset = m_stroke_width / 2.0;
    double r = std::max(0.0, static_cast<double>(m_corner_radius) - offset);
    draw_rounded_rect(cr, bounds.x + offset, bounds.y + offset,
                      bounds.width - m_stroke_width, bounds.height - m_stroke_width, r);
    cairo_set_source_rgba(cr, m_stroke_color.r, m_stroke_color.g, m_stroke_color.b, m_stroke_color.a);
    cairo_set_line_width(cr, m_stroke_width);
    cairo_stroke(cr);
    cairo_restore(cr);
}

void ViewGroup::add_view(std::shared_ptr<View> child) {
    if (!child) return;
    if (m_window) child->set_window(m_window);
    m_children.push_back(std::move(child));
}

void ViewGroup::add_view(std::shared_ptr<View> child, const LayoutParams& params) {
    if (!child) return;
    child->set_layout_params(params);
    add_view(std::move(child));
}

void ViewGroup::remove_view(std::shared_ptr<View> child) {
    auto it = std::remove(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it, m_children.end());
    }
}

void ViewGroup::clear_views() {
    m_children.clear();
    m_child_entries.clear();
}

void ViewGroup::set_window(Window* win) {
    View::set_window(win);
    for (auto& child : m_children) {
        if (child) child->set_window(win);
    }
}

bool ViewGroup::on_mouse_move(int lx, int ly, const Rect& bounds) {
    if (!is_visible()) return false;
    bool handled = false;

    for (auto& entry : m_child_entries) {
        if (entry.view && entry.view->is_visible()) {
            if (entry.view->on_mouse_move(lx, ly, entry.allocated_bounds)) {
                handled = true;
            }
        }
    }
    return handled;
}

bool ViewGroup::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible()) return false;

    // Traverse top-to-bottom (reverse order)
    for (auto it = m_child_entries.rbegin(); it != m_child_entries.rend(); ++it) {
        if (it->view && it->view->is_visible()) {
            if (!it->allocated_bounds.contains(Point(lx, ly))) {
                continue;
            }
            if (it->view->on_mouse_button(lx, ly, button, pressed, it->allocated_bounds)) {
                return true;
            }
        }
    }

    if (m_on_click && button == MouseButton::Left && bounds.contains(Point(lx, ly))) {
        if (pressed) {
            m_pressed = true;
            return true;
        } else if (m_pressed) {
            m_pressed = false;
            m_on_click();
            return true;
        }
    }

    if (!pressed) {
        m_pressed = false;
    }

    return false;
}

bool ViewGroup::on_key(const KeyPressEvent& event) {
    if (!is_visible()) return false;

    for (auto& child : m_children) {
        if (child && child->is_visible()) {
            if (child->on_key(event)) {
                return true;
            }
        }
    }
    return false;
}

bool ViewGroup::on_scroll(double delta) {
    if (!is_visible()) return false;

    for (auto& child : m_children) {
        if (child && child->is_visible()) {
            if (child->on_scroll(delta)) {
                return true;
            }
        }
    }
    return false;
}

bool ViewGroup::on_touch(const TouchEvent& event, const Rect& bounds) {
    if (!is_visible()) return false;

    if (event.phase == TouchPhase::Down) {
        m_touch_target = nullptr;
        // Traverse top-to-bottom (reverse order)
        for (auto it = m_child_entries.rbegin(); it != m_child_entries.rend(); ++it) {
            if (it->view && it->view->is_visible()) {
                if (!it->allocated_bounds.contains(Point(static_cast<int>(event.x), static_cast<int>(event.y)))) {
                    continue;
                }
                if (it->view->on_touch(event, it->allocated_bounds)) {
                    m_touch_target = it->view;
                    m_touch_target_bounds = it->allocated_bounds;
                    return true;
                }
            }
        }
        if (m_on_click && bounds.contains(Point(static_cast<int>(event.x), static_cast<int>(event.y)))) {
            return View::on_touch(event, bounds);
        }
        return false;
    }

    if (m_touch_target) {
        Rect target_bounds = m_touch_target_bounds;
        for (const auto& entry : m_child_entries) {
            if (entry.view == m_touch_target) {
                target_bounds = entry.allocated_bounds;
                break;
            }
        }

        bool handled = m_touch_target->on_touch(event, target_bounds);
        if (event.phase == TouchPhase::Up || event.phase == TouchPhase::Cancel) {
            m_touch_target = nullptr;
        }
        return handled;
    }

    if (m_on_click) {
        return View::on_touch(event, bounds);
    }

    return false;
}

} // namespace miqu
