#include "miqutoolkit/view/view_pager.hpp"
#include "miqutoolkit/core/window.hpp"
#include <algorithm>

namespace miqu {

void ViewPager::add_page(std::shared_ptr<View> page) {
    if (!page) return;
    m_pages.push_back(page);
    ViewGroup::add_view(page);
}

void ViewPager::set_current_page(int index) {
    if (m_pages.empty()) return;
    int target = std::clamp(index, 0, static_cast<int>(m_pages.size()) - 1);
    if (target != m_current_page) {
        int old = m_current_page;
        m_current_page = target;
        if (m_on_page_changed) {
            m_on_page_changed(old, m_current_page);
        }
        if (m_window) m_window->schedule_redraw();
    }
}

std::shared_ptr<View> ViewPager::get_page(int index) const {
    if (index >= 0 && index < static_cast<int>(m_pages.size())) {
        return m_pages[index];
    }
    return nullptr;
}

Size ViewPager::measure_size() const {
    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        Size sz = m_pages[m_current_page]->measure_size();
        return Size(sz.width + m_padding.left + m_padding.right, sz.height + m_padding.top + m_padding.bottom);
    }
    return Size(m_padding.left + m_padding.right, m_padding.top + m_padding.bottom);
}

void ViewPager::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    draw_background(cr, bounds);

    m_child_entries.clear();
    Rect content_rect = get_content_rect(bounds);

    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        auto& active_page = m_pages[m_current_page];
        if (active_page && active_page->is_visible() && content_rect.width > 0 && content_rect.height > 0) {
            cairo_save(cr);
            cairo_rectangle(cr, content_rect.x, content_rect.y, content_rect.width, content_rect.height);
            cairo_clip(cr);

            m_child_entries.push_back({active_page, content_rect});
            active_page->draw(cr, content_rect);

            cairo_restore(cr);
        }
    }

    draw_stroke(cr, bounds);
}

bool ViewPager::on_mouse_move(int lx, int ly, const Rect& bounds) {
    if (!is_visible()) return false;
    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        auto& active_page = m_pages[m_current_page];
        if (active_page && active_page->is_visible()) {
            Rect content_rect = get_content_rect(bounds);
            return active_page->on_mouse_move(lx, ly, content_rect);
        }
    }
    return false;
}

bool ViewPager::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (!is_visible()) return false;
    Rect content_rect = get_content_rect(bounds);
    if (!content_rect.contains(Point(lx, ly))) return false;

    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        auto& active_page = m_pages[m_current_page];
        if (active_page && active_page->is_visible()) {
            return active_page->on_mouse_button(lx, ly, button, pressed, content_rect);
        }
    }
    return false;
}

bool ViewPager::on_scroll(double delta) {
    if (!is_visible()) return false;
    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        auto& active_page = m_pages[m_current_page];
        if (active_page && active_page->is_visible()) {
            return active_page->on_scroll(delta);
        }
    }
    return false;
}

bool ViewPager::on_key(const KeyPressEvent& event) {
    if (!is_visible()) return false;
    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        auto& active_page = m_pages[m_current_page];
        if (active_page && active_page->is_visible()) {
            return active_page->on_key(event);
        }
    }
    return false;
}

bool ViewPager::on_touch(const TouchEvent& event, const Rect& bounds) {
    if (!is_visible()) return false;
    Rect content_rect = get_content_rect(bounds);
    if (m_current_page >= 0 && m_current_page < static_cast<int>(m_pages.size())) {
        auto& active_page = m_pages[m_current_page];
        if (active_page && active_page->is_visible()) {
            return active_page->on_touch(event, content_rect);
        }
    }
    return false;
}

ViewPagerBuilder::ViewPagerBuilder() : m_view(std::make_shared<ViewPager>()) {}

} // namespace miqu
