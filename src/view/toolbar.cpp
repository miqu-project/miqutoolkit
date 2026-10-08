#include "miqutoolkit/view/toolbar.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <algorithm>

namespace miqu {

Toolbar::Toolbar() {
    m_layout_params = LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    );

    // Default padding
    m_padding = Padding(0, 0, 0, 0);

    // Title & Subtitle text views
    m_title_view = TextViewBuilder::create()
        ->h2()
        ->bold(true)
        ->ellipsize(true)
        ->build();
    m_title_view->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    m_subtitle_view = TextViewBuilder::create()
        ->caption()
        ->muted()
        ->ellipsize(true)
        ->build();
    m_subtitle_view->set_layout_params(LayoutParams(
        static_cast<int>(LayoutDimension::MatchParent),
        static_cast<int>(LayoutDimension::WrapContent)
    ));

    // Standard Buttons (Back, Refresh, Menu, Close)
    m_btn_back = std::make_shared<ImageButton>(icons::BACK);
    m_btn_back->set_circle(true);
    m_btn_back->set_icon_size(20);
    m_btn_back->set_layout_params(LayoutParams(40, 40, Gravity::CenterVertical));
    m_btn_back->set_on_click_listener([this]() {
        if (m_on_back) m_on_back();
    });

    m_btn_refresh = std::make_shared<ImageButton>(icons::REFRESH);
    m_btn_refresh->set_circle(true);
    m_btn_refresh->set_icon_size(18);
    m_btn_refresh->set_layout_params(LayoutParams(40, 40, Gravity::CenterVertical));
    m_btn_refresh->set_on_click_listener([this]() {
        if (m_on_refresh) m_on_refresh();
    });

    m_btn_menu = std::make_shared<ImageButton>(icons::MENU);
    m_btn_menu->set_circle(true);
    m_btn_menu->set_icon_size(18);
    m_btn_menu->set_layout_params(LayoutParams(40, 40, Gravity::CenterVertical));
    m_btn_menu->set_on_click_listener([this]() {
        if (m_on_menu) m_on_menu();
    });

    m_btn_close = std::make_shared<ImageButton>(icons::POWER);
    m_btn_close->set_circle(true);
    m_btn_close->set_icon_size(18);
    m_btn_close->set_layout_params(LayoutParams(40, 40, Gravity::CenterVertical));
    m_btn_close->set_on_click_listener([this]() {
        if (m_on_close) {
            m_on_close();
        } else if (m_window) {
            m_window->request_close();
        }
    });

    rebuild_internal_views();
}

void Toolbar::set_window(Window* win) {
    ViewGroup::set_window(win);
    if (m_btn_back) m_btn_back->set_window(win);
    if (m_icon_view) m_icon_view->set_window(win);
    if (m_icon_img_view) m_icon_img_view->set_window(win);
    if (m_leading_custom_view) m_leading_custom_view->set_window(win);
    if (m_title_view) m_title_view->set_window(win);
    if (m_subtitle_view) m_subtitle_view->set_window(win);
    if (m_center_view) m_center_view->set_window(win);
    if (m_btn_refresh) m_btn_refresh->set_window(win);
    if (m_btn_menu) m_btn_menu->set_window(win);
    if (m_btn_close) m_btn_close->set_window(win);
    for (auto& a : m_custom_actions) {
        if (a) a->set_window(win);
    }
}

void Toolbar::rebuild_internal_views() {
    m_children.clear();

    if (m_back_enabled && m_back_visible && m_btn_back) m_children.push_back(m_btn_back);
    if (m_icon_view && !m_icon_text.empty()) m_children.push_back(m_icon_view);
    if (m_icon_img_view && !m_icon_resource.empty()) m_children.push_back(m_icon_img_view);
    if (m_leading_custom_view) m_children.push_back(m_leading_custom_view);

    if (m_center_view) {
        m_children.push_back(m_center_view);
    } else {
        if (m_title_view && !m_title.empty()) m_children.push_back(m_title_view);
        if (m_subtitle_view && !m_subtitle.empty()) m_children.push_back(m_subtitle_view);
    }

    for (auto& a : m_custom_actions) {
        if (a) m_children.push_back(a);
    }

    if (m_refresh_visible && m_btn_refresh) m_children.push_back(m_btn_refresh);
    if (m_menu_visible && m_btn_menu) m_children.push_back(m_btn_menu);
    if (m_close_visible && m_btn_close) m_children.push_back(m_btn_close);

    if (m_window) {
        for (auto& child : m_children) {
            if (child) child->set_window(m_window);
        }
    }
}

void Toolbar::set_back_button(bool enable, std::function<void()> on_back) {
    m_back_enabled = enable;
    m_back_visible = enable;
    if (on_back) m_on_back = std::move(on_back);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_back_visible(bool visible) {
    m_back_visible = visible;
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

bool Toolbar::is_back_visible() const {
    return m_back_enabled && m_back_visible;
}

void Toolbar::set_icon_text(const std::string& icon_text) {
    m_icon_text = icon_text;
    if (!m_icon_text.empty()) {
        if (!m_icon_view) {
            m_icon_view = TextViewBuilder::create()
                ->h2()
                ->build();
        }
        m_icon_view->set_text(m_icon_text);
    }
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_icon_resource(const std::string& resource_path) {
    m_icon_resource = resource_path;
    if (!m_icon_resource.empty()) {
        if (!m_icon_img_view) {
            m_icon_img_view = ImageViewBuilder::create()
                ->imageResource(m_icon_resource)
                ->targetSize(32)
                ->build();
            m_icon_img_view->set_layout_params(LayoutParams(32, 32, Gravity::CenterVertical));
        } else {
            m_icon_img_view->set_image_resource(m_icon_resource);
        }
    }
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_leading_view(std::shared_ptr<View> view) {
    m_leading_custom_view = std::move(view);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_title(const std::string& title) {
    m_title = title;
    if (m_title_view) {
        m_title_view->set_text(m_title);
    }
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_subtitle(const std::string& subtitle) {
    m_subtitle = subtitle;
    if (m_subtitle_view) {
        m_subtitle_view->set_text(m_subtitle);
    }
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_title_alignment(TitleAlignment alignment) {
    m_title_alignment = alignment;
    if (m_title_view) {
        m_title_view->set_text_alignment(alignment == TitleAlignment::Center ? TextAlignment::Center : TextAlignment::Left);
    }
    if (m_subtitle_view) {
        m_subtitle_view->set_text_alignment(alignment == TitleAlignment::Center ? TextAlignment::Center : TextAlignment::Left);
    }
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_center_view(std::shared_ptr<View> view) {
    m_center_view = std::move(view);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

std::shared_ptr<ImageButton> Toolbar::add_action(const std::string& icon, std::function<void()> on_click) {
    auto btn = std::make_shared<ImageButton>(icon);
    btn->set_circle(true);
    btn->set_icon_size(18);
    btn->set_layout_params(LayoutParams(40, 40, Gravity::CenterVertical));
    if (on_click) btn->set_on_click_listener(std::move(on_click));
    m_custom_actions.push_back(btn);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
    return btn;
}

void Toolbar::add_action_view(std::shared_ptr<View> view) {
    if (view) {
        m_custom_actions.push_back(std::move(view));
        rebuild_internal_views();
        if (m_window) m_window->schedule_redraw();
    }
}

void Toolbar::clear_custom_actions() {
    m_custom_actions.clear();
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_on_menu(std::function<void()> on_menu) {
    m_on_menu = std::move(on_menu);
    m_menu_visible = (m_on_menu != nullptr);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_menu_visible(bool visible) {
    m_menu_visible = visible;
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_on_refresh(std::function<void()> on_refresh) {
    m_on_refresh = std::move(on_refresh);
    m_refresh_visible = (m_on_refresh != nullptr);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_refresh_visible(bool visible) {
    m_refresh_visible = visible;
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_on_close(std::function<void()> on_close) {
    m_on_close = std::move(on_close);
    m_close_visible = (m_on_close != nullptr);
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

void Toolbar::set_close_visible(bool visible) {
    m_close_visible = visible;
    rebuild_internal_views();
    if (m_window) m_window->schedule_redraw();
}

Size Toolbar::measure_size() const {
    return measure_size(-1);
}

Size Toolbar::measure_size(int avail_width) const {
    int w = (avail_width >= 0) ? avail_width : 400;
    int h = m_toolbar_height;

    if (!m_subtitle.empty()) {
        h = std::max(h, 52);
    }
    h += m_padding.top + m_padding.bottom;
    return Size(w, h);
}

void Toolbar::draw(cairo_t* cr, const Rect& bounds) {
    if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

    draw_background(cr, bounds);

    m_child_entries.clear();

    int inner_x = bounds.x + m_padding.left;
    int inner_y = bounds.y + m_padding.top;
    int inner_w = std::max(0, bounds.width - m_padding.left - m_padding.right);
    int inner_h = std::max(0, bounds.height - m_padding.top - m_padding.bottom);

    if (inner_w <= 0 || inner_h <= 0) return;

    // 1. Measure and place Leading items from left to right
    int cur_x = inner_x;

    auto place_leading = [&](const std::shared_ptr<View>& view, int w, int h) {
        if (!view || !view->is_visible()) return;
        int vy = inner_y + (inner_h - h) / 2;
        Rect r(cur_x, vy, w, h);
        m_child_entries.push_back({view, r});
        view->draw(cr, r);
        cur_x += w + 8; // spacing
    };

    if (m_back_enabled && m_back_visible && m_btn_back) {
        place_leading(m_btn_back, 40, 40);
    }

    if (!m_icon_text.empty() && m_icon_view) {
        Size sz = m_icon_view->measure_size(-1);
        int iw = sz.width > 0 ? sz.width : 28;
        int ih = sz.height > 0 ? sz.height : 28;
        place_leading(m_icon_view, iw, ih);
    } else if (!m_icon_resource.empty() && m_icon_img_view) {
        int target_s = m_icon_img_view->get_target_size() > 0 ? m_icon_img_view->get_target_size() : 32;
        place_leading(m_icon_img_view, target_s, target_s);
    }

    if (m_leading_custom_view && m_leading_custom_view->is_visible()) {
        Size sz = m_leading_custom_view->measure_size(-1);
        int lw = (m_leading_custom_view->get_layout_params().width > 0) ? m_leading_custom_view->get_layout_params().width : sz.width;
        int lh = (m_leading_custom_view->get_layout_params().height > 0) ? m_leading_custom_view->get_layout_params().height : sz.height;
        place_leading(m_leading_custom_view, lw, lh);
    }

    int leading_end_x = cur_x;

    // 2. Measure and place Trailing items from left to right at the far right
    struct ActionItem {
        std::shared_ptr<View> view;
        int width;
        int height;
    };
    std::vector<ActionItem> trailing_items;

    for (const auto& a : m_custom_actions) {
        if (!a || !a->is_visible()) continue;
        Size sz = a->measure_size(-1);
        int aw = (a->get_layout_params().width > 0) ? a->get_layout_params().width : (sz.width > 0 ? sz.width : 40);
        int ah = (a->get_layout_params().height > 0) ? a->get_layout_params().height : (sz.height > 0 ? sz.height : 40);
        trailing_items.push_back({a, aw, ah});
    }

    if (m_refresh_visible && m_btn_refresh) {
        trailing_items.push_back({m_btn_refresh, 40, 40});
    }

    if (m_menu_visible && m_btn_menu) {
        trailing_items.push_back({m_btn_menu, 40, 40});
    }

    if (m_close_visible && m_btn_close) {
        trailing_items.push_back({m_btn_close, 40, 40});
    }

    int trailing_total_w = 0;
    for (size_t i = 0; i < trailing_items.size(); ++i) {
        trailing_total_w += trailing_items[i].width;
        if (i + 1 < trailing_items.size()) trailing_total_w += 6; // 6px gap
    }

    int trailing_start_x = inner_x + inner_w - trailing_total_w;
    int cur_tx = trailing_start_x;

    for (size_t i = 0; i < trailing_items.size(); ++i) {
        const auto& item = trailing_items[i];
        int vy = inner_y + (inner_h - item.height) / 2;
        Rect r(cur_tx, vy, item.width, item.height);
        m_child_entries.push_back({item.view, r});
        item.view->draw(cr, r);
        cur_tx += item.width + 6;
    }

    // 3. Center / Title Section
    int left_bound = leading_end_x + (leading_end_x > inner_x ? 8 : 0);
    int right_bound = (trailing_total_w > 0) ? (trailing_start_x - 8) : (inner_x + inner_w);
    int avail_center_w = std::max(0, right_bound - left_bound);

    if (avail_center_w > 0) {
        if (m_center_view && m_center_view->is_visible()) {
            Size cs = m_center_view->measure_size(avail_center_w);
            int cw = (m_center_view->get_layout_params().width > 0) ? m_center_view->get_layout_params().width : cs.width;
            if (cw <= 0 || cw > avail_center_w) cw = avail_center_w;
            int ch = (m_center_view->get_layout_params().height > 0) ? m_center_view->get_layout_params().height : cs.height;
            if (ch <= 0 || ch > inner_h) ch = inner_h;

            int cx = left_bound;
            if (m_title_alignment == TitleAlignment::Center) {
                cx = inner_x + (inner_w - cw) / 2;
                if (cx < left_bound) cx = left_bound;
                if (cx + cw > right_bound) cw = std::max(0, right_bound - cx);
            }
            int cy = inner_y + (inner_h - ch) / 2;
            Rect r(cx, cy, cw, ch);
            m_child_entries.push_back({m_center_view, r});
            m_center_view->draw(cr, r);
        } else {
            // Title & Subtitle Stack
            bool has_title = (!m_title.empty() && m_title_view);
            bool has_subtitle = (!m_subtitle.empty() && m_subtitle_view);

            if (has_title && has_subtitle) {
                Size t_sz = m_title_view->measure_size(avail_center_w);
                Size s_sz = m_subtitle_view->measure_size(avail_center_w);
                int th = t_sz.height > 0 ? t_sz.height : 22;
                int sh = s_sz.height > 0 ? s_sz.height : 16;
                int total_text_h = th + 2 + sh;

                int start_y = inner_y + (inner_h - total_text_h) / 2;

                int cx = left_bound;
                int cw = avail_center_w;
                if (m_title_alignment == TitleAlignment::Center) {
                    int content_max_w = std::max(t_sz.width, s_sz.width);
                    cw = std::min(avail_center_w, content_max_w);
                    cx = inner_x + (inner_w - cw) / 2;
                    if (cx < left_bound) {
                        cx = left_bound;
                        cw = std::max(0, right_bound - cx);
                    }
                    if (cx + cw > right_bound) {
                        cw = std::max(0, right_bound - cx);
                    }
                }

                Rect title_r(cx, start_y, cw, th);
                Rect sub_r(cx, start_y + th + 2, cw, sh);

                m_child_entries.push_back({m_title_view, title_r});
                m_title_view->draw(cr, title_r);

                m_child_entries.push_back({m_subtitle_view, sub_r});
                m_subtitle_view->draw(cr, sub_r);
            } else if (has_title) {
                Size t_sz = m_title_view->measure_size(avail_center_w);
                int th = t_sz.height > 0 ? t_sz.height : 24;
                int start_y = inner_y + (inner_h - th) / 2;

                int cx = left_bound;
                int cw = avail_center_w;
                if (m_title_alignment == TitleAlignment::Center) {
                    cw = std::min(avail_center_w, t_sz.width);
                    cx = inner_x + (inner_w - cw) / 2;
                    if (cx < left_bound) {
                        cx = left_bound;
                        cw = std::max(0, right_bound - cx);
                    }
                    if (cx + cw > right_bound) {
                        cw = std::max(0, right_bound - cx);
                    }
                }

                Rect title_r(cx, start_y, cw, th);
                m_child_entries.push_back({m_title_view, title_r});
                m_title_view->draw(cr, title_r);
            }
        }
    }

    // 4. Optional subtle divider line at bottom
    if (m_show_divider) {
        cairo_save(cr);
        auto config = Config::get();
        Color div_col = config->colors.outline_variant.with_alpha(0.35f);
        cairo_set_source_rgba(cr, div_col.r, div_col.g, div_col.b, div_col.a);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, bounds.x, bounds.y + bounds.height - 0.5);
        cairo_line_to(cr, bounds.x + bounds.width, bounds.y + bounds.height - 0.5);
        cairo_stroke(cr);
        cairo_restore(cr);
    }
}

} // namespace miqu
