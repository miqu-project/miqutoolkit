#include "miqutoolkit/view/view_group.hpp"
#include "miqutoolkit/view/popup_menu.hpp"
#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/core/config.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace miqu {

View::~View() {
    if (m_window) {
        m_window->notify_view_destroyed(this);
    }
}

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

void View::draw_focus_ring(cairo_t* cr, const Rect& bounds) {
    if (!m_draw_focus_ring || !cr || bounds.width <= 0 || bounds.height <= 0) return;
    auto config = Config::get();
    Color focus_color = config ? config->colors.primary : Color(0.2f, 0.6f, 1.0f);

    cairo_save(cr);
    cairo_set_source_rgba(cr, focus_color.r, focus_color.g, focus_color.b, 0.90f);
    cairo_set_line_width(cr, 2.0);

    double offset = 1.0;
    double x = bounds.x + offset;
    double y = bounds.y + offset;
    double w = bounds.width - (offset * 2.0);
    double h = bounds.height - (offset * 2.0);
    double r = (m_focus_ring_radius >= 0) ? m_focus_ring_radius : (config ? config->metrics.corner_radius : 8.0);

    ViewGroup::draw_rounded_rect(cr, x, y, w, h, r);
    cairo_stroke(cr);
    cairo_restore(cr);

    if (m_window) {
        m_window->notify_focus_ring_drawn();
    }
}

std::shared_ptr<View> View::find_view_at(int lx, int ly, Rect* out_bounds) {
    if (!is_visible()) return nullptr;
    Rect r = (m_bounds.width > 0 && m_bounds.height > 0) ? m_bounds : (out_bounds ? *out_bounds : Rect());
    if (!r.contains(Point(lx, ly))) return nullptr;
    if (out_bounds) *out_bounds = r;
    return shared_from_this();
}

void View::set_context_menu(std::shared_ptr<PopupMenu> menu) {
    m_context_menu = std::move(menu);
}

View* View::contextMenu(std::shared_ptr<PopupMenu> menu) {
    m_context_menu = std::move(menu);
    return this;
}

void View::set_context_menu_builder(std::function<std::shared_ptr<PopupMenu>(int, int)> builder) {
    m_context_menu_builder = std::move(builder);
}

View* View::contextMenuBuilder(std::function<std::shared_ptr<PopupMenu>(int, int)> builder) {
    m_context_menu_builder = std::move(builder);
    return this;
}

bool View::handle_context_click(int lx, int ly, const Rect& bounds) {
    (void)bounds;
    if (m_context_menu_builder) {
        auto menu = m_context_menu_builder(lx, ly);
        if (menu && m_window) {
            m_active_context_menu = menu;
            auto prev_cb = menu->get_on_dismiss();
            menu->set_on_dismiss([this, prev_cb]() {
                if (prev_cb) prev_cb();
                m_active_context_menu = nullptr;
            });
            menu->show_at_location(m_window, lx, ly);
            return true;
        }
    } else if (m_context_menu && m_window) {
        m_context_menu->show_at_location(m_window, lx, ly);
        return true;
    }
    if (m_on_context_click) {
        m_on_context_click(lx, ly);
        return true;
    }
    return false;
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
    if (m_stroke_width > 0 && m_stroke_color.a > 0.0f) {
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

    if (has_focus() && should_draw_focus_ring()) {
        const_cast<ViewGroup*>(this)->draw_focus_ring(cr, bounds);
    }
}

void ViewGroup::draw_focus_ring(cairo_t* cr, const Rect& bounds) {
    if (!should_draw_focus_ring() || !cr || bounds.width <= 0 || bounds.height <= 0) return;
    auto config = Config::get();
    Color focus_color = config ? config->colors.primary : Color(0.2f, 0.6f, 1.0f);

    cairo_save(cr);
    cairo_set_source_rgba(cr, focus_color.r, focus_color.g, focus_color.b, 0.90f);
    cairo_set_line_width(cr, 2.0);

    double offset = 1.0;
    double x = bounds.x + offset;
    double y = bounds.y + offset;
    double w = bounds.width - (offset * 2.0);
    double h = bounds.height - (offset * 2.0);
    double r = (m_focus_ring_radius >= 0) ? m_focus_ring_radius :
               ((m_corner_radius > 0) ? m_corner_radius : (config ? config->metrics.corner_radius : 8.0));

    draw_rounded_rect(cr, x, y, w, h, r);
    cairo_stroke(cr);
    cairo_restore(cr);

    if (m_window) {
        m_window->notify_focus_ring_drawn();
    }
}

void ViewGroup::add_view(std::shared_ptr<View> child) {
    if (!child) return;
    child->set_parent(this);
    if (m_window) child->set_window(m_window);
    m_children.push_back(std::move(child));
}

void ViewGroup::add_view(std::shared_ptr<View> child, const LayoutParams& params) {
    if (!child) return;
    child->set_layout_params(params);
    add_view(std::move(child));
}

void ViewGroup::remove_view(std::shared_ptr<View> child) {
    if (!child) return;
    if (m_mouse_target == child) {
        m_mouse_target = nullptr;
    }
    if (m_touch_target == child) {
        m_touch_target = nullptr;
    }
    child->set_parent(nullptr);
    auto it = std::remove(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        m_children.erase(it, m_children.end());
    }
}

void ViewGroup::clear_views() {
    m_touch_target = nullptr;
    m_mouse_target = nullptr;
    for (auto& c : m_children) {
        if (c) c->set_parent(nullptr);
    }
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

    if (m_mouse_target) {
        Rect target_bounds = m_mouse_target_bounds;
        for (const auto& entry : m_child_entries) {
            if (entry.view == m_mouse_target) {
                target_bounds = entry.allocated_bounds;
                break;
            }
        }
        handled = m_mouse_target->on_mouse_move(lx, ly, target_bounds);
    } else {
        for (auto& entry : m_child_entries) {
            if (entry.view && entry.view->is_visible()) {
                if (entry.view->on_mouse_move(lx, ly, entry.allocated_bounds)) {
                    handled = true;
                }
            }
        }
    }
    return handled;
}

bool ViewGroup::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible()) return false;
    auto self = shared_from_this();

    if (!pressed && m_mouse_target) {
        Rect target_bounds = m_mouse_target_bounds;
        for (const auto& entry : m_child_entries) {
            if (entry.view == m_mouse_target) {
                target_bounds = entry.allocated_bounds;
                break;
            }
        }
        auto target = m_mouse_target;
        m_mouse_target = nullptr;
        bool handled = target->on_mouse_button(lx, ly, button, false, target_bounds);
        if (m_pressed) {
            m_pressed = false;
        }
        return handled;
    }

    if (pressed) {
        m_mouse_target = nullptr;
        // Traverse top-to-bottom on a safe snapshot copy
        auto entries_copy = m_child_entries;
        for (auto it = entries_copy.rbegin(); it != entries_copy.rend(); ++it) {
            auto child = it->view;
            Rect child_bounds = it->allocated_bounds;
            if (child && child->is_visible()) {
                if (!child_bounds.contains(Point(lx, ly))) {
                    continue;
                }
                if (child->on_mouse_button(lx, ly, button, true, child_bounds)) {
                    // Check if child is still a member of this ViewGroup after event dispatch
                    for (const auto& cur : m_child_entries) {
                        if (cur.view == child) {
                            m_mouse_target = child;
                            m_mouse_target_bounds = cur.allocated_bounds;
                            break;
                        }
                    }
                    return true;
                }
            }
        }
    } else {
        auto entries_copy = m_child_entries;
        for (auto it = entries_copy.rbegin(); it != entries_copy.rend(); ++it) {
            auto child = it->view;
            Rect child_bounds = it->allocated_bounds;
            if (child && child->is_visible()) {
                if (!child_bounds.contains(Point(lx, ly))) {
                    continue;
                }
                if (child->on_mouse_button(lx, ly, button, false, child_bounds)) {
                    return true;
                }
            }
        }
    }

    if (m_on_click && button == MouseButton::Left && bounds.contains(Point(lx, ly))) {
        if (pressed) {
            m_pressed = true;
            return true;
        } else if (m_pressed) {
            m_pressed = false;
            auto cb = m_on_click;
            cb();
            return true;
        }
    }

    if (button == MouseButton::Right && pressed && bounds.contains(Point(lx, ly))) {
        if (handle_context_click(lx, ly, bounds)) {
            return true;
        }
    }

    if (!pressed) {
        m_pressed = false;
    }

    return false;
}

bool ViewGroup::on_key(const KeyPressEvent& event) {
    if (!is_visible() || !is_enabled()) return false;
    auto self = shared_from_this();

    // If this ViewGroup itself has focus and a click listener, handle Enter / Space
    if (has_focus() && event.pressed &&
        (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter || event.keysym == XKB_KEY_space)) {
        if (m_on_click) {
            auto cb = m_on_click;
            request_redraw();
            cb();
            return true;
        }
    }

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
    auto self = shared_from_this();

    if (event.phase == TouchPhase::Down) {
        m_touch_target = nullptr;
        auto entries_copy = m_child_entries;
        for (auto it = entries_copy.rbegin(); it != entries_copy.rend(); ++it) {
            auto child = it->view;
            Rect child_bounds = it->allocated_bounds;
            if (child && child->is_visible()) {
                if (!child_bounds.contains(Point(static_cast<int>(event.x), static_cast<int>(event.y)))) {
                    continue;
                }
                if (child->on_touch(event, child_bounds)) {
                    for (const auto& cur : m_child_entries) {
                        if (cur.view == child) {
                            m_touch_target = child;
                            m_touch_target_bounds = cur.allocated_bounds;
                            break;
                        }
                    }
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
        auto target = m_touch_target;
        if (event.phase == TouchPhase::Up || event.phase == TouchPhase::Cancel) {
            m_touch_target = nullptr;
        }

        Rect target_bounds = m_touch_target_bounds;
        for (const auto& entry : m_child_entries) {
            if (entry.view == target) {
                target_bounds = entry.allocated_bounds;
                break;
            }
        }

        bool handled = target->on_touch(event, target_bounds);
        return handled;
    }

    if (m_on_click) {
        return View::on_touch(event, bounds);
    }

    return false;
}

std::shared_ptr<View> ViewGroup::find_view_at(int lx, int ly, Rect* out_bounds) {
    if (!is_visible()) return nullptr;

    for (auto it = m_child_entries.rbegin(); it != m_child_entries.rend(); ++it) {
        if (it->view && it->view->is_visible() && it->allocated_bounds.contains(Point(lx, ly))) {
            Rect child_bounds = it->allocated_bounds;
            auto found = it->view->find_view_at(lx, ly, &child_bounds);
            if (found) {
                if (found->has_tooltip()) {
                    if (out_bounds) *out_bounds = child_bounds;
                    return found;
                }
                if (has_tooltip()) {
                    if (out_bounds) *out_bounds = (m_bounds.width > 0 && m_bounds.height > 0) ? m_bounds : it->allocated_bounds;
                    return shared_from_this();
                }
                if (out_bounds) *out_bounds = child_bounds;
                return found;
            }
        }
    }

    if (m_bounds.contains(Point(lx, ly))) {
        if (out_bounds) *out_bounds = m_bounds;
        return shared_from_this();
    }
    return nullptr;
}

void ViewGroup::collect_focusable_views(std::vector<View*>& out) {
    if (!is_visible() || !is_enabled()) return;
    for (const auto& child : m_children) {
        if (!child || !child->is_visible() || !child->is_enabled()) continue;
        if (child->is_focusable()) {
            out.push_back(child.get());
        }
        if (auto group = std::dynamic_pointer_cast<ViewGroup>(child)) {
            group->collect_focusable_views(out);
        }
    }
}

} // namespace miqu
