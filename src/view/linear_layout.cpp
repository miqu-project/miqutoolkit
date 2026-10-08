#include "miqutoolkit/view/linear_layout.hpp"
#include <numeric>
#include <algorithm>

namespace miqu {

Size LinearLayout::measure_size() const {
    return measure_size(-1);
}

Size LinearLayout::measure_size(int avail_width) const {
    int total_w = 0;
    int total_h = 0;
    int visible_child_count = 0;

    int inner_w = (avail_width >= 0) ? std::max(0, avail_width - m_padding.left - m_padding.right) : -1;

    if (m_orientation == Orientation::Horizontal && inner_w >= 0) {
        // Horizontal orientation with known available width: calculate weights first
        float total_weight = 0.0f;
        int used_fixed = 0;

        for (const auto& child : m_children) {
            if (!child || child->get_visibility() == Visibility::Gone) continue;
            const auto& params = child->get_layout_params();
            const auto& margin = child->get_margin();

            if (params.weight > 0.0f) {
                total_weight += params.weight;
                used_fixed += margin.left + margin.right;
            } else {
                Size child_size = child->measure_size(-1);
                int child_w = (params.width >= 0) ? params.width : child_size.width;
                used_fixed += child_w + margin.left + margin.right;
            }
            visible_child_count++;
        }

        if (visible_child_count > 1) {
            used_fixed += (visible_child_count - 1) * m_spacing;
        }

        int remaining = std::max(0, inner_w - used_fixed);

        // Second pass: measure children with their allocated width
        for (const auto& child : m_children) {
            if (!child || child->get_visibility() == Visibility::Gone) continue;
            const auto& params = child->get_layout_params();
            const auto& margin = child->get_margin();

            int child_w = 0;
            int child_avail_w = -1;

            if (params.weight > 0.0f && total_weight > 0.0f) {
                child_w = static_cast<int>((params.weight / total_weight) * remaining);
                child_avail_w = child_w;
            } else if (params.width >= 0) {
                child_w = params.width;
                child_avail_w = child_w;
            }

            Size child_size = child->measure_size(child_avail_w);
            if (child_w <= 0) {
                child_w = (params.width >= 0) ? params.width : child_size.width;
            }
            int child_h = (params.height >= 0) ? params.height : child_size.height;

            total_w += child_w + margin.left + margin.right;
            total_h = std::max(total_h, child_h + margin.top + margin.bottom);
        }

        if (visible_child_count > 1) {
            total_w += (visible_child_count - 1) * m_spacing;
        }
    } else {
        for (const auto& child : m_children) {
            if (!child || child->get_visibility() == Visibility::Gone) continue;

            const auto& params = child->get_layout_params();
            const auto& margin = child->get_margin();

            int child_avail_w = -1;
            if (m_orientation == Orientation::Vertical) {
                child_avail_w = (inner_w >= 0) ? std::max(0, inner_w - margin.left - margin.right) : -1;
            }

            Size child_size = child->measure_size(child_avail_w);

            int child_w = (params.width >= 0) ? params.width : child_size.width;
            int child_h = (params.height >= 0) ? params.height : child_size.height;

            int child_total_w = child_w + margin.left + margin.right;
            int child_total_h = child_h + margin.top + margin.bottom;

            if (m_orientation == Orientation::Horizontal) {
                total_w += child_total_w;
                total_h = std::max(total_h, child_total_h);
            } else {
                total_w = std::max(total_w, child_total_w);
                total_h += child_total_h;
            }
            visible_child_count++;
        }

        if (visible_child_count > 1) {
            if (m_orientation == Orientation::Horizontal) {
                total_w += (visible_child_count - 1) * m_spacing;
            } else {
                total_h += (visible_child_count - 1) * m_spacing;
            }
        }
    }

    total_w += m_padding.left + m_padding.right;
    total_h += m_padding.top + m_padding.bottom;

    return Size(total_w, total_h);
}

void LinearLayout::draw(cairo_t* cr, const Rect& bounds) {
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

    if (avail_w <= 0 || avail_h <= 0) return;

    // First pass: Calculate fixed space and total weight
    float total_weight = 0.0f;
    int used_fixed_space = 0;
    int visible_child_count = 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();

        if (params.weight > 0.0f) {
            total_weight += params.weight;
            if (m_orientation == Orientation::Horizontal) {
                used_fixed_space += margin.left + margin.right;
            } else {
                used_fixed_space += margin.top + margin.bottom;
            }
        } else {
            Size measured = child->measure_size();
            if (m_orientation == Orientation::Horizontal) {
                int child_w = (params.width >= 0) ? params.width : measured.width;
                used_fixed_space += child_w + margin.left + margin.right;
            } else {
                int child_h = (params.height >= 0) ? params.height : measured.height;
                used_fixed_space += child_h + margin.top + margin.bottom;
            }
        }
        visible_child_count++;
    }

    if (visible_child_count > 1) {
        used_fixed_space += (visible_child_count - 1) * m_spacing;
    }

    int total_avail_axis = (m_orientation == Orientation::Horizontal) ? avail_w : avail_h;
    int remaining_space = std::max(0, total_avail_axis - used_fixed_space);

    // Second pass: Allocate and draw
    int current_cursor = 0;

    double clip_x1 = 0, clip_y1 = 0, clip_x2 = 0, clip_y2 = 0;
    bool has_clip = (cr != nullptr);
    if (has_clip) {
        cairo_clip_extents(cr, &clip_x1, &clip_y1, &clip_x2, &clip_y2);
    }

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        const auto& params = child->get_layout_params();
        const auto& margin = child->get_margin();

        int child_w = 0;
        int child_h = 0;

        if (m_orientation == Orientation::Horizontal) {
            int child_avail_w = std::max(0, avail_w - margin.left - margin.right);
            if (params.weight > 0.0f && total_weight > 0.0f) {
                child_w = static_cast<int>((params.weight / total_weight) * remaining_space);
            } else if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
                child_w = child_avail_w;
            } else if (params.width >= 0) {
                child_w = params.width;
            } else {
                Size measured = child->measure_size();
                if (measured.width > 0) {
                    child_w = measured.width;
                } else {
                    child_w = child_avail_w;
                }
            }

            int child_avail_h = std::max(0, avail_h - margin.top - margin.bottom);
            Size child_m = child->measure_size(child_w);

            if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
                child_h = child_avail_h;
            } else if (params.height >= 0) {
                child_h = params.height;
            } else if (child_m.height > 0) {
                child_h = child_m.height;
            } else {
                child_h = child_avail_h;
            }

            int child_x = origin_x + current_cursor + margin.left;
            int child_y = origin_y + margin.top;

            Gravity g = (params.gravity != Gravity::None) ? params.gravity : m_gravity;
            if (g & Gravity::CenterVertical) {
                child_y = origin_y + (avail_h - child_h) / 2;
            } else if (g & Gravity::Bottom) {
                child_y = origin_y + avail_h - child_h - margin.bottom;
            }

            Rect child_bounds(child_x, child_y, child_w, child_h);
            child->set_bounds(child_bounds);
            m_child_entries.push_back({child, child_bounds});

            if (!has_clip || (clip_x2 <= clip_x1) ||
                (child_x + child_w >= clip_x1 && child_x <= clip_x2 &&
                 child_y + child_h >= clip_y1 && child_y <= clip_y2)) {
                child->draw(cr, child_bounds);
            }

            current_cursor += child_w + margin.left + margin.right + m_spacing;
        } else {
            // Vertical Orientation
            int child_avail_w = std::max(0, avail_w - margin.left - margin.right);
            int child_avail_h = std::max(0, avail_h - margin.top - margin.bottom);
            Size measured = child->measure_size(child_avail_w);

            if (params.weight > 0.0f && total_weight > 0.0f) {
                child_h = static_cast<int>((params.weight / total_weight) * remaining_space);
            } else if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
                child_h = child_avail_h;
            } else if (params.height >= 0) {
                child_h = params.height;
            } else if (measured.height > 0) {
                child_h = measured.height;
            } else {
                child_h = child_avail_h;
            }

            if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
                child_w = child_avail_w;
            } else if (params.width >= 0) {
                child_w = std::min(params.width, child_avail_w);
            } else if (measured.width > 0) {
                child_w = std::min(measured.width, child_avail_w);
            } else {
                child_w = child_avail_w;
            }

            int child_x = origin_x + margin.left;
            int child_y = origin_y + current_cursor + margin.top;

            Gravity g = (params.gravity != Gravity::None) ? params.gravity : m_gravity;
            if (g & Gravity::CenterHorizontal) {
                child_x = origin_x + (avail_w - child_w) / 2;
            } else if (g & Gravity::Right) {
                child_x = origin_x + avail_w - child_w - margin.right;
            }

            Rect child_bounds(child_x, child_y, child_w, child_h);
            child->set_bounds(child_bounds);
            m_child_entries.push_back({child, child_bounds});

            if (!has_clip || (clip_y2 <= clip_y1) ||
                (child_x + child_w >= clip_x1 && child_x <= clip_x2 &&
                 child_y + child_h >= clip_y1 && child_y <= clip_y2)) {
                child->draw(cr, child_bounds);
            }

            current_cursor += child_h + margin.top + margin.bottom + m_spacing;
        }
    }

    if (needs_clip) {
        cairo_restore(cr);
    }
    draw_stroke(cr, bounds);
}

} // namespace miqu
