#include "miqutoolkit/view/resizable_container.hpp"
#include <algorithm>

namespace miqu {

ResizableContainer::ResizableContainer() = default;

ResizableContainer::ResizableContainer(std::shared_ptr<View> content) {
    set_content(std::move(content));
}

void ResizableContainer::set_content(std::shared_ptr<View> content) {
    clear_views();
    m_content = std::move(content);
    if (m_content) {
        add_view(m_content);
    }
}

bool ResizableContainer::is_in_resize_grip(int lx, int ly, const Rect& bounds) const {
    if (!m_resizable || bounds.width <= 0 || bounds.height <= 0) return false;
    return (lx >= (bounds.x + bounds.width - m_grip_size) &&
            ly >= (bounds.y + bounds.height - m_grip_size) &&
            bounds.contains(lx, ly));
}

void ResizableContainer::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    Rect content_bounds = get_content_rect(bounds);

    if (has_background()) {
        draw_background(cr, content_bounds);
    }

    if (m_content && m_content->is_visible()) {
        m_content->draw(cr, content_bounds);
    }

    // Subtle resize grip in bottom-right corner
    if (m_resizable) {
        cairo_save(cr);
        double grip_alpha = (m_active || m_hover_resize) ? 0.80 : 0.22;
        cairo_set_source_rgba(cr, 1.0, 1.0, 1.0, grip_alpha);
        cairo_set_line_width(cr, 1.5);
        cairo_set_line_cap(cr, CAIRO_LINE_CAP_ROUND);

        int rx = bounds.x + bounds.width;
        int ry = bounds.y + bounds.height;
        // Outer diagonal grip tick
        cairo_move_to(cr, rx - 14, ry - 6);
        cairo_line_to(cr, rx - 6, ry - 14);
        // Inner diagonal grip tick
        cairo_move_to(cr, rx - 10, ry - 6);
        cairo_line_to(cr, rx - 6, ry - 10);
        cairo_stroke(cr);
        cairo_restore(cr);
    }

    // Active dashed highlight outline when dragging or resizing
    if (m_active) {
        cairo_save(cr);
        cairo_set_source_rgba(cr, m_active_color.r, m_active_color.g, m_active_color.b, m_active_color.a);
        cairo_set_line_width(cr, 2.0);
        double dashes[] = { 6.0, 4.0 };
        cairo_set_dash(cr, dashes, 2, 0.0);
        cairo_rectangle(cr, bounds.x - 1, bounds.y - 1, bounds.width + 2, bounds.height + 2);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

bool ResizableContainer::on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) {
    if (m_content && m_content->is_visible()) {
        Rect content_bounds = get_content_rect(bounds);
        return m_content->on_mouse_button(lx, ly, button, pressed, content_bounds);
    }
    return false;
}

bool ResizableContainer::on_mouse_move(int lx, int ly, const Rect& bounds) {
    m_hover_resize = is_in_resize_grip(lx, ly, bounds);

    if (m_content && m_content->is_visible()) {
        Rect content_bounds = get_content_rect(bounds);
        return m_content->on_mouse_move(lx, ly, content_bounds);
    }
    return false;
}

} // namespace miqu
