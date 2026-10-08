#include "miqutoolkit/view/relative_layout.hpp"
#include <map>

namespace miqu {

Size RelativeLayout::measure_size() const {
    int max_w = 0;
    int max_h = 0;

    for (const auto& child : m_children) {
        if (!child || child->get_visibility() == Visibility::Gone) continue;

        Size child_size = child->measure_size();
        const auto& margin = child->get_margin();
        int child_total_w = child_size.width + margin.left + margin.right;
        int child_total_h = child_size.height + margin.top + margin.bottom;

        max_w = std::max(max_w, child_total_w);
        max_h = std::max(max_h, child_total_h);
    }

    max_w += m_padding.left + m_padding.right;
    max_h += m_padding.top + m_padding.bottom;

    return Size(max_w, max_h);
}

void RelativeLayout::draw(cairo_t* cr, const Rect& bounds) {
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

    std::vector<std::shared_ptr<View>> pending;
    for (const auto& child : m_children) {
        if (child && child->get_visibility() != Visibility::Gone) {
            pending.push_back(child);
        }
    }

    std::map<View*, Rect> resolved_bounds;

    while (!pending.empty()) {
        bool progress = false;
        for (auto it = pending.begin(); it != pending.end(); ) {
            const auto& child = *it;
            const auto& params = child->get_layout_params();

            bool dep_ready = true;
            if (params.to_end_of && resolved_bounds.find(params.to_end_of.get()) == resolved_bounds.end()) {
                dep_ready = false;
            }
            if (params.to_start_of && resolved_bounds.find(params.to_start_of.get()) == resolved_bounds.end()) {
                dep_ready = false;
            }
            if (params.below && resolved_bounds.find(params.below.get()) == resolved_bounds.end()) {
                dep_ready = false;
            }
            if (params.above && resolved_bounds.find(params.above.get()) == resolved_bounds.end()) {
                dep_ready = false;
            }

            if (dep_ready) {
                const auto& margin = child->get_margin();
                Size measured = child->measure_size();

                int child_w = (params.width >= 0) ? params.width : measured.width;
                int child_h = (params.height >= 0) ? params.height : measured.height;

                if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
                    child_w = std::max(0, avail_w - margin.left - margin.right);
                }
                if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
                    child_h = std::max(0, avail_h - margin.top - margin.bottom);
                }

                int child_x = origin_x + margin.left;
                int child_y = origin_y + margin.top;

                // Sibling horizontal positioning
                if (params.to_end_of && resolved_bounds.find(params.to_end_of.get()) != resolved_bounds.end()) {
                    const auto& ref = resolved_bounds[params.to_end_of.get()];
                    child_x = ref.x + ref.width + margin.left;
                } else if (params.to_start_of && resolved_bounds.find(params.to_start_of.get()) != resolved_bounds.end()) {
                    const auto& ref = resolved_bounds[params.to_start_of.get()];
                    child_x = ref.x - child_w - margin.right;
                } else if (params.align_parent_end) {
                    child_x = origin_x + avail_w - child_w - margin.right;
                } else if (params.center_horizontal || params.center_in_parent) {
                    child_x = origin_x + (avail_w - child_w) / 2;
                }

                // Sibling vertical positioning
                if (params.below && resolved_bounds.find(params.below.get()) != resolved_bounds.end()) {
                    const auto& ref = resolved_bounds[params.below.get()];
                    child_y = ref.y + ref.height + margin.top;
                } else if (params.above && resolved_bounds.find(params.above.get()) != resolved_bounds.end()) {
                    const auto& ref = resolved_bounds[params.above.get()];
                    child_y = ref.y - child_h - margin.bottom;
                } else if (params.align_parent_bottom) {
                    child_y = origin_y + avail_h - child_h - margin.bottom;
                } else if (params.center_vertical || params.center_in_parent) {
                    child_y = origin_y + (avail_h - child_h) / 2;
                }

                Rect child_rect(child_x, child_y, child_w, child_h);
                child->set_bounds(child_rect);
                resolved_bounds[child.get()] = child_rect;
                m_child_entries.push_back({child, child_rect});

                it = pending.erase(it);
                progress = true;
            } else {
                ++it;
            }
        }

        if (!progress && !pending.empty()) {
            auto child = pending.front();
            pending.erase(pending.begin());
            const auto& params = child->get_layout_params();
            const auto& margin = child->get_margin();
            Size measured = child->measure_size();

            int child_w = (params.width >= 0) ? params.width : measured.width;
            int child_h = (params.height >= 0) ? params.height : measured.height;
            if (params.width == static_cast<int>(LayoutDimension::MatchParent)) {
                child_w = std::max(0, avail_w - margin.left - margin.right);
            }
            if (params.height == static_cast<int>(LayoutDimension::MatchParent)) {
                child_h = std::max(0, avail_h - margin.top - margin.bottom);
            }

            int child_x = origin_x + margin.left;
            int child_y = origin_y + margin.top;
            if (params.align_parent_end) child_x = origin_x + avail_w - child_w - margin.right;
            else if (params.center_horizontal || params.center_in_parent) child_x = origin_x + (avail_w - child_w) / 2;
            if (params.align_parent_bottom) child_y = origin_y + avail_h - child_h - margin.bottom;
            else if (params.center_vertical || params.center_in_parent) child_y = origin_y + (avail_h - child_h) / 2;

            Rect child_rect(child_x, child_y, child_w, child_h);
            child->set_bounds(child_rect);
            resolved_bounds[child.get()] = child_rect;
            m_child_entries.push_back({child, child_rect});
        }
    }

    for (const auto& entry : m_child_entries) {
        if (entry.view && entry.view->is_visible()) {
            entry.view->draw(cr, entry.allocated_bounds);
        }
    }

    if (needs_clip) {
        cairo_restore(cr);
    }
    draw_stroke(cr, bounds);
}

} // namespace miqu
