#include "miqutoolkit/view/frame_layout.hpp"

namespace miqu {

Size FrameLayout::measure_size() const {
    return measure_size(-1);
}

Size FrameLayout::measure_size(int avail_width) const {
    int inner_w = (avail_width >= 0) ? std::max(0, avail_width - m_padding.left - m_padding.right) : -1;
    int max_w = (m_layout_params.width >= 0) ? m_layout_params.width : 0;
    int max_h = (m_layout_params.height >= 0) ? m_layout_params.height : 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();
        int child_avail_w = (inner_w >= 0) ? std::max(0, inner_w - margin.left - margin.right) : -1;
        Size child_size = child->measure_size(child_avail_w);

        int child_w = (params.width >= 0) ? params.width : child_size.width;
        int child_h = (params.height >= 0) ? params.height : child_size.height;

        int child_total_w = child_w + margin.left + margin.right;
        int child_total_h = child_h + margin.top + margin.bottom;

        max_w = std::max(max_w, child_total_w);
        max_h = std::max(max_h, child_total_h);
    }

    max_w += m_padding.left + m_padding.right;
    max_h += m_padding.top + m_padding.bottom;

    return Size(max_w, max_h);
}

void FrameLayout::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    draw_background(cr, bounds);

    bool needs_clip = (m_corner_radius > 0);
    if (needs_clip) {
        cairo_save(cr);
        draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, m_corner_radius);
        cairo_clip(cr);
    }

    m_child_entries.clear();

    Rect content_rect = get_content_rect(bounds);
    int avail_w = content_rect.width;
    int avail_h = content_rect.height;
    int origin_x = content_rect.x;
    int origin_y = content_rect.y;

    if (avail_w <= 0 || avail_h <= 0) {
        if (needs_clip) cairo_restore(cr);
        draw_stroke(cr, bounds);
        return;
    }

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();
        int child_avail_w = std::max(0, avail_w - margin.left - margin.right);
        int child_avail_h = std::max(0, avail_h - margin.top - margin.bottom);
        Size measured = child->measure_size(child_avail_w);

        int child_w = 0;
        int child_h = 0;

        if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
            child_w = child_avail_w;
        } else if (params.width >= 0) {
            child_w = std::min(params.width, child_avail_w);
        } else if (measured.width > 0) {
            child_w = std::min(measured.width, child_avail_w);
        } else {
            child_w = child_avail_w;
        }

        if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
            child_h = child_avail_h;
        } else if (params.height >= 0) {
            child_h = params.height;
        } else if (measured.height > 0) {
            child_h = measured.height;
        } else {
            child_h = child_avail_h;
        }

        int child_x = origin_x + margin.left;
        int child_y = origin_y + margin.top;

        Gravity g = params.gravity;
        if (g & Gravity::CenterHorizontal) {
            child_x = origin_x + (avail_w - child_w) / 2;
        } else if (g & Gravity::Right) {
            child_x = origin_x + avail_w - child_w - margin.right;
        }

        if (g & Gravity::CenterVertical) {
            child_y = origin_y + (avail_h - child_h) / 2;
        } else if (g & Gravity::Bottom) {
            child_y = origin_y + avail_h - child_h - margin.bottom;
        }

        Rect child_bounds(child_x, child_y, child_w, child_h);
        child->set_bounds(child_bounds);
        m_child_entries.push_back({child, child_bounds});
        child->draw(cr, child_bounds);
    }

    if (needs_clip) {
        cairo_restore(cr);
    }
    draw_stroke(cr, bounds);
}

} // namespace miqu
