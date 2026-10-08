#include "miqutoolkit/view/navigation_view.hpp"
#include "miqutoolkit/core/window.hpp"
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>

namespace miqu {

const std::string NavigationView::s_empty_str = "";

void NavigationView::push(std::shared_ptr<View> view, const std::string& title, const std::string& tag) {
    push(std::move(view), title, "", tag);
}

void NavigationView::push(std::shared_ptr<View> view, const std::string& title, const std::string& subtitle, const std::string& tag) {
    if (!view) return;

    if (view->get_layout_params().width == static_cast<int>(LayoutDimension::WrapContent) ||
        view->get_layout_params().width == 0) {
        auto params = view->get_layout_params();
        params.width = static_cast<int>(LayoutDimension::MatchParent);
        params.height = static_cast<int>(LayoutDimension::MatchParent);
        view->set_layout_params(params);
    }

    if (m_window) {
        view->set_window(m_window);
    }

    ViewGroup::add_view(view);
    m_stack.push_back({view, title, subtitle, tag});

    notify_navigation_changed();
}

bool NavigationView::pop() {
    if (m_stack.size() <= 1) {
        return false;
    }

    auto popped = m_stack.back();
    m_stack.pop_back();

    if (popped.view) {
        ViewGroup::remove_view(popped.view);
    }

    notify_navigation_changed();
    return true;
}

bool NavigationView::pop_to_root() {
    if (m_stack.size() <= 1) {
        return false;
    }

    while (m_stack.size() > 1) {
        auto popped = m_stack.back();
        m_stack.pop_back();
        if (popped.view) {
            ViewGroup::remove_view(popped.view);
        }
    }

    notify_navigation_changed();
    return true;
}

bool NavigationView::pop_to_tag(const std::string& tag) {
    if (m_stack.size() <= 1 || tag.empty()) {
        return false;
    }

    int target_idx = -1;
    for (int i = static_cast<int>(m_stack.size()) - 1; i >= 0; --i) {
        if (m_stack[i].tag == tag) {
            target_idx = i;
            break;
        }
    }

    if (target_idx < 0 || target_idx == static_cast<int>(m_stack.size()) - 1) {
        return false;
    }

    while (static_cast<int>(m_stack.size()) > target_idx + 1) {
        auto popped = m_stack.back();
        m_stack.pop_back();
        if (popped.view) {
            ViewGroup::remove_view(popped.view);
        }
    }

    notify_navigation_changed();
    return true;
}

void NavigationView::replace_top(std::shared_ptr<View> view, const std::string& title, const std::string& tag) {
    if (!view) return;

    if (!m_stack.empty()) {
        auto popped = m_stack.back();
        m_stack.pop_back();
        if (popped.view) {
            ViewGroup::remove_view(popped.view);
        }
    }

    push(std::move(view), title, tag);
}

std::shared_ptr<View> NavigationView::get_current_view() const {
    if (!m_stack.empty()) {
        return m_stack.back().view;
    }
    return nullptr;
}

const std::string& NavigationView::get_current_title() const {
    if (!m_stack.empty()) {
        return m_stack.back().title;
    }
    return s_empty_str;
}

const std::string& NavigationView::get_current_subtitle() const {
    if (!m_stack.empty()) {
        return m_stack.back().subtitle;
    }
    return s_empty_str;
}

const std::string& NavigationView::get_current_tag() const {
    if (!m_stack.empty()) {
        return m_stack.back().tag;
    }
    return s_empty_str;
}

const NavigationPage* NavigationView::get_current_page() const {
    if (!m_stack.empty()) {
        return &m_stack.back();
    }
    return nullptr;
}

std::shared_ptr<View> NavigationView::get_root_view() const {
    if (!m_stack.empty()) {
        return m_stack.front().view;
    }
    return nullptr;
}

void NavigationView::notify_navigation_changed() {
    if (m_on_navigation && !m_stack.empty()) {
        m_on_navigation(m_stack.back(), can_go_back());
    }
    if (m_window) {
        m_window->schedule_redraw();
    }
}

void NavigationView::set_window(Window* win) {
    ViewGroup::set_window(win);
    for (auto& page : m_stack) {
        if (page.view) {
            page.view->set_window(win);
        }
    }
}

Size NavigationView::measure_size() const {
    return measure_size(-1);
}

Size NavigationView::measure_size(int avail_width) const {
    if (!m_stack.empty() && m_stack.back().view) {
        int inner_w = (avail_width >= 0) ? std::max(0, avail_width - m_padding.left - m_padding.right) : -1;
        Size sz = m_stack.back().view->measure_size(inner_w);
        return Size(sz.width + m_padding.left + m_padding.right, sz.height + m_padding.top + m_padding.bottom);
    }
    return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);
}

void NavigationView::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    draw_background(cr, bounds);

    m_child_entries.clear();
    Rect content_rect = get_content_rect(bounds);

    if (!m_stack.empty()) {
        auto& top = m_stack.back();
        if (top.view && top.view->is_visible() && content_rect.width > 0 && content_rect.height > 0) {
            m_child_entries.push_back({top.view, content_rect});
            top.view->draw(cr, content_rect);
        }
    }

    draw_stroke(cr, bounds);
}

bool NavigationView::on_mouse_move(int lx, int ly, const Rect& bounds) {
    if (!is_visible() || m_stack.empty()) return false;
    Rect content_rect = get_content_rect(bounds);
    auto& top = m_stack.back();
    if (top.view && top.view->is_visible()) {
        return top.view->on_mouse_move(lx, ly, content_rect);
    }
    return false;
}

bool NavigationView::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible() || m_stack.empty()) return false;
    Rect content_rect = get_content_rect(bounds);
    auto& top = m_stack.back();

    if (top.view && top.view->is_visible()) {
        if (top.view->on_mouse_button(lx, ly, button, pressed, content_rect)) {
            return true;
        }
    }

    // Automated mouse back button (Thumb button)
    if (pressed && m_auto_back && button == MouseButton::Back && can_go_back()) {
        pop();
        return true;
    }

    return false;
}

bool NavigationView::on_scroll(double delta) {
    if (!is_visible() || m_stack.empty()) return false;
    auto& top = m_stack.back();
    if (top.view && top.view->is_visible()) {
        return top.view->on_scroll(delta);
    }
    return false;
}

bool NavigationView::on_touch(const TouchEvent& event, const Rect& bounds) {
    if (!is_visible() || m_stack.empty()) return false;
    Rect content_rect = get_content_rect(bounds);
    auto& top = m_stack.back();
    if (top.view && top.view->is_visible()) {
        return top.view->on_touch(event, content_rect);
    }
    return false;
}

bool NavigationView::on_key(const KeyPressEvent& event) {
    if (!is_visible() || m_stack.empty()) return false;
    auto& top = m_stack.back();

    if (top.view && top.view->is_visible()) {
        if (top.view->on_key(event)) {
            return true;
        }
    }

    // Automated keyboard back triggers (Escape or Alt + Left)
    if (event.pressed && m_auto_back && can_go_back()) {
        if (m_back_on_escape && event.keysym == XKB_KEY_Escape) {
            pop();
            return true;
        }
        if (event.has_alt() && (event.keysym == XKB_KEY_Left || event.keysym == XKB_KEY_BackSpace)) {
            pop();
            return true;
        }
    }

    return false;
}

} // namespace miqu
