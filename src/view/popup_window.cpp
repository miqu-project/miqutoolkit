#include "miqutoolkit/view/popup_window.hpp"
#include "miqutoolkit/view/frame_layout.hpp"
#include "miqutoolkit/core/window.hpp"
#include "miqutoolkit/core/config.hpp"
#include <algorithm>
#include <cmath>

namespace miqu {

namespace {

/**
 * @brief Internal container view that provides elevated ambient drop shadow,
 * rounded background fill, subtle outline border, and content clipping.
 */
class PopupContainerView : public FrameLayout {
public:
    PopupContainerView() {
        set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::MatchParent)
        ));
    }

    void set_elevation_level(int elev) { m_elev = elev; }
    void set_border_style(double width, const Color& col) {
        m_border_w = width;
        m_border_col = col;
    }
    void set_custom_bg(const Color& col, bool has) {
        m_custom_bg = col;
        m_has_bg = has;
    }
    void set_radius(int r) { m_radius_override = r; }

    void draw(cairo_t* cr, const Rect& bounds) override {
        if (!is_visible() || !cr || bounds.width <= 0 || bounds.height <= 0) return;

        auto config = Config::get();
        int radius = (m_radius_override >= 0) ? m_radius_override : config->metrics.corner_radius;
        Color bg = m_has_bg ? m_custom_bg : config->colors.surface;
        Color stroke_col = (m_border_col.a > 0.0f) ? m_border_col : config->colors.outline_variant;
        double stroke_w = (m_border_w >= 0.0) ? m_border_w : 1.0;

        // 1. Multi-pass ambient alpha drop shadow
        if (m_elev > 0) {
            cairo_save(cr);
            int steps = std::min(m_elev, 5);
            for (int i = 1; i <= steps; ++i) {
                double shadow_offset = i * 1.6;
                double shadow_expand = (i - 1) * 0.75;
                double alpha = 0.045 / std::sqrt(static_cast<double>(i));
                draw_rounded_rect(cr, bounds.x - shadow_expand, bounds.y + shadow_offset,
                                  bounds.width + shadow_expand * 2.0, bounds.height,
                                  radius + shadow_expand);
                cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, alpha);
                cairo_fill(cr);
            }
            cairo_restore(cr);
        }

        // 2. Background fill
        cairo_save(cr);
        draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
        cairo_set_source_rgba(cr, bg.r, bg.g, bg.b, bg.a);
        cairo_fill(cr);
        cairo_restore(cr);

        // 3. Child content clipping and drawing
        bool clip_corners = (radius > 0);
        if (clip_corners) {
            cairo_save(cr);
            draw_rounded_rect(cr, bounds.x, bounds.y, bounds.width, bounds.height, radius);
            cairo_clip(cr);
        }

        FrameLayout::draw(cr, bounds);

        if (clip_corners) {
            cairo_restore(cr);
        }

        // 4. Subtle outline border stroke on top of content
        if (stroke_w > 0.0 && stroke_col.a > 0.0f) {
            cairo_save(cr);
            double offset = stroke_w / 2.0;
            double r = std::max(0.0, static_cast<double>(radius) - offset);
            draw_rounded_rect(cr, bounds.x + offset, bounds.y + offset,
                              bounds.width - stroke_w, bounds.height - stroke_w, r);
            cairo_set_source_rgba(cr, stroke_col.r, stroke_col.g, stroke_col.b, stroke_col.a);
            cairo_set_line_width(cr, stroke_w);
            cairo_stroke(cr);
            cairo_restore(cr);
        }
    }

private:
    int m_elev = 4;
    int m_radius_override = -1;
    double m_border_w = 1.0;
    Color m_border_col;
    Color m_custom_bg;
    bool m_has_bg = false;
};

} // anonymous namespace

PopupWindow::PopupWindow() = default;

PopupWindow::PopupWindow(std::shared_ptr<View> content)
    : m_content_view(std::move(content)) {}

PopupWindow::PopupWindow(std::shared_ptr<View> content, int width, int height)
    : m_content_view(std::move(content)), m_width(width), m_height(height) {}

PopupWindow::~PopupWindow() {
    dismiss();
}

void PopupWindow::set_content_view(std::shared_ptr<View> content) {
    m_content_view = std::move(content);
    if (m_container_view) {
        auto* container = dynamic_cast<PopupContainerView*>(m_container_view.get());
        if (container) {
            container->clear_views();
            if (m_content_view) {
                container->add_view(m_content_view);
            }
        }
    }
}

void PopupWindow::set_size(int width, int height) {
    m_width = width;
    m_height = height;
}

void PopupWindow::set_elevation(int elevation) {
    m_elevation = std::max(0, elevation);
    if (m_container_view) {
        if (auto* c = dynamic_cast<PopupContainerView*>(m_container_view.get())) {
            c->set_elevation_level(m_elevation);
        }
    }
}

void PopupWindow::set_corner_radius(int radius) {
    m_corner_radius = radius;
    if (m_container_view) {
        if (auto* c = dynamic_cast<PopupContainerView*>(m_container_view.get())) {
            c->set_radius(m_corner_radius);
        }
    }
}

void PopupWindow::set_background_color(const Color& color) {
    m_bg_color = color;
    m_has_bg_color = true;
    if (m_container_view) {
        if (auto* c = dynamic_cast<PopupContainerView*>(m_container_view.get())) {
            c->set_custom_bg(m_bg_color, true);
        }
    }
}

void PopupWindow::set_border(double stroke_width, const Color& stroke_color) {
    m_stroke_width = stroke_width;
    m_stroke_color = stroke_color;
    m_has_stroke_color = true;
    if (m_container_view) {
        if (auto* c = dynamic_cast<PopupContainerView*>(m_container_view.get())) {
            c->set_border_style(m_stroke_width, m_stroke_color);
        }
    }
}

void PopupWindow::set_padding(const Padding& padding) {
    m_padding = padding;
    if (m_container_view) {
        m_container_view->set_padding(m_padding);
    }
}

void PopupWindow::set_padding(int uniform) {
    set_padding(Padding(uniform));
}

void PopupWindow::set_padding(int horizontal, int vertical) {
    set_padding(Padding(horizontal, vertical));
}

void PopupWindow::set_padding(int left, int top, int right, int bottom) {
    set_padding(Padding(left, top, right, bottom));
}

void PopupWindow::ensure_container_view() {
    if (!m_container_view) {
        auto container = std::make_shared<PopupContainerView>();
        container->set_elevation_level(m_elevation);
        container->set_radius(m_corner_radius);
        if (m_has_bg_color) container->set_custom_bg(m_bg_color, true);
        if (m_has_stroke_color) container->set_border_style(m_stroke_width, m_stroke_color);
        container->set_padding(m_padding);
        if (m_content_view) {
            container->add_view(m_content_view);
        }
        m_container_view = container;
    }
}

Size PopupWindow::measure_popup_size(Window* window, const Rect& anchor_rect) const {
    int w = m_width;
    int h = m_height;

    if (w == static_cast<int>(LayoutDimension::MatchParent)) {
        w = anchor_rect.width;
    } else if (w == static_cast<int>(LayoutDimension::WrapContent) || w <= 0) {
        if (m_content_view) {
            Size measured = m_content_view->measure_size();
            w = measured.width + m_padding.left + m_padding.right;
        } else {
            w = 140;
        }
    }

    if (h == static_cast<int>(LayoutDimension::MatchParent)) {
        h = anchor_rect.height;
    } else if (h == static_cast<int>(LayoutDimension::WrapContent) || h <= 0) {
        if (m_content_view) {
            Size measured = m_content_view->measure_size(w);
            h = measured.height + m_padding.top + m_padding.bottom;
        } else {
            h = 40;
        }
    }

    if (m_max_width > 0 && w > m_max_width) w = m_max_width;
    if (m_max_height > 0 && h > m_max_height) h = m_max_height;

    if (window) {
        int max_win_w = std::max(60, window->get_width() - (m_window_margin * 2));
        int max_win_h = std::max(40, window->get_height() - (m_window_margin * 2));
        w = std::min(w, max_win_w);
        h = std::min(h, max_win_h);
    }

    return Size(std::max(w, 20), std::max(h, 20));
}

Rect PopupWindow::calculate_raw_anchor_bounds(const Rect& anchor_rect,
                                              const Size& popup_size,
                                              PopupGravity gravity,
                                              int x_offset, int y_offset) const {
    int w = popup_size.width;
    int h = popup_size.height;
    int x = 0;
    int y = 0;

    switch (gravity) {
        case PopupGravity::BottomStart:
            x = anchor_rect.x + x_offset;
            y = anchor_rect.y + anchor_rect.height + y_offset;
            break;
        case PopupGravity::BottomEnd:
            x = anchor_rect.x + anchor_rect.width - w + x_offset;
            y = anchor_rect.y + anchor_rect.height + y_offset;
            break;
        case PopupGravity::BottomCenter:
            x = anchor_rect.x + (anchor_rect.width - w) / 2 + x_offset;
            y = anchor_rect.y + anchor_rect.height + y_offset;
            break;
        case PopupGravity::TopStart:
            x = anchor_rect.x + x_offset;
            y = anchor_rect.y - h + y_offset;
            break;
        case PopupGravity::TopEnd:
            x = anchor_rect.x + anchor_rect.width - w + x_offset;
            y = anchor_rect.y - h + y_offset;
            break;
        case PopupGravity::TopCenter:
            x = anchor_rect.x + (anchor_rect.width - w) / 2 + x_offset;
            y = anchor_rect.y - h + y_offset;
            break;
        case PopupGravity::StartTop:
            x = anchor_rect.x - w + x_offset;
            y = anchor_rect.y + y_offset;
            break;
        case PopupGravity::StartBottom:
            x = anchor_rect.x - w + x_offset;
            y = anchor_rect.y + anchor_rect.height - h + y_offset;
            break;
        case PopupGravity::StartCenter:
            x = anchor_rect.x - w + x_offset;
            y = anchor_rect.y + (anchor_rect.height - h) / 2 + y_offset;
            break;
        case PopupGravity::EndTop:
            x = anchor_rect.x + anchor_rect.width + x_offset;
            y = anchor_rect.y + y_offset;
            break;
        case PopupGravity::EndBottom:
            x = anchor_rect.x + anchor_rect.width + x_offset;
            y = anchor_rect.y + anchor_rect.height - h + y_offset;
            break;
        case PopupGravity::EndCenter:
            x = anchor_rect.x + anchor_rect.width + x_offset;
            y = anchor_rect.y + (anchor_rect.height - h) / 2 + y_offset;
            break;
        case PopupGravity::Center:
            x = anchor_rect.x + (anchor_rect.width - w) / 2 + x_offset;
            y = anchor_rect.y + (anchor_rect.height - h) / 2 + y_offset;
            break;
    }

    return Rect(x, y, w, h);
}

Rect PopupWindow::apply_boundary_flip(const Rect& raw_rect,
                                      const Rect& anchor_rect,
                                      const Size& popup_size,
                                      const Size& window_size,
                                      PopupGravity gravity,
                                      int x_offset, int y_offset) const {
    Rect result = raw_rect;
    int w = popup_size.width;
    int h = popup_size.height;
    int win_w = window_size.width;
    int win_h = window_size.height;

    bool is_bottom = (gravity == PopupGravity::BottomStart ||
                      gravity == PopupGravity::BottomEnd ||
                      gravity == PopupGravity::BottomCenter);
    bool is_top = (gravity == PopupGravity::TopStart ||
                   gravity == PopupGravity::TopEnd ||
                   gravity == PopupGravity::TopCenter);

    // 1. Vertical Flip: bottom -> top if overflowing bottom and space above exists
    if (is_bottom && result.y + h > win_h - m_window_margin) {
        int flipped_y = anchor_rect.y - h - y_offset;
        if (flipped_y >= m_window_margin) {
            result.y = flipped_y;
        }
    } else if (is_top && result.y < m_window_margin) {
        int flipped_y = anchor_rect.y + anchor_rect.height - y_offset;
        if (flipped_y + h <= win_h - m_window_margin) {
            result.y = flipped_y;
        }
    }

    // 2. Horizontal Flip for dropdowns: right overflow -> flip alignment to end
    if (result.x + w > win_w - m_window_margin) {
        int flipped_x = anchor_rect.x + anchor_rect.width - w + x_offset;
        if (flipped_x >= m_window_margin) {
            result.x = flipped_x;
        }
    } else if (result.x < m_window_margin) {
        int flipped_x = anchor_rect.x + x_offset;
        if (flipped_x + w <= win_w - m_window_margin) {
            result.x = flipped_x;
        }
    }

    // 3. Side alignments: Start -> End, End -> Start
    bool is_start = (gravity == PopupGravity::StartTop ||
                     gravity == PopupGravity::StartBottom ||
                     gravity == PopupGravity::StartCenter);
    bool is_end = (gravity == PopupGravity::EndTop ||
                   gravity == PopupGravity::EndBottom ||
                   gravity == PopupGravity::EndCenter);

    if (is_start && result.x < m_window_margin) {
        int flipped_x = anchor_rect.x + anchor_rect.width + x_offset;
        if (flipped_x + w <= win_w - m_window_margin) {
            result.x = flipped_x;
        }
    } else if (is_end && result.x + w > win_w - m_window_margin) {
        int flipped_x = anchor_rect.x - w - x_offset;
        if (flipped_x >= m_window_margin) {
            result.x = flipped_x;
        }
    }

    return result;
}

Rect PopupWindow::clamp_to_viewport(const Rect& rect, const Size& window_size) const {
    int max_x = std::max(m_window_margin, window_size.width - rect.width - m_window_margin);
    int max_y = std::max(m_window_margin, window_size.height - rect.height - m_window_margin);

    int clamped_x = std::clamp(rect.x, m_window_margin, max_x);
    int clamped_y = std::clamp(rect.y, m_window_margin, max_y);

    return Rect(clamped_x, clamped_y, rect.width, rect.height);
}

void PopupWindow::show_as_dropdown(const std::shared_ptr<View>& anchor,
                                   PopupGravity gravity,
                                   int x_offset, int y_offset) {
    if (!anchor) return;
    show_as_dropdown(anchor.get(), gravity, x_offset, y_offset);
}

void PopupWindow::show_as_dropdown(View* anchor,
                                   PopupGravity gravity,
                                   int x_offset, int y_offset) {
    if (!anchor) return;
    Window* win = anchor->get_window();
    if (!win) return;

    ensure_container_view();

    Rect anchor_rect = anchor->get_bounds();
    if (anchor_rect.width <= 0 || anchor_rect.height <= 0) {
        Size measured = anchor->measure_size();
        if (anchor_rect.width <= 0) anchor_rect.width = std::max(measured.width, 20);
        if (anchor_rect.height <= 0) anchor_rect.height = std::max(measured.height, 20);
    }
    Size win_size(win->get_width(), win->get_height());
    Size popup_size = measure_popup_size(win, anchor_rect);

    Rect calculated = calculate_raw_anchor_bounds(anchor_rect, popup_size, gravity, x_offset, y_offset);

    if (m_auto_flip) {
        calculated = apply_boundary_flip(calculated, anchor_rect, popup_size, win_size, gravity, x_offset, y_offset);
    }

    if (m_auto_clamp) {
        calculated = clamp_to_viewport(calculated, win_size);
    }

    m_bounds = calculated;
    m_window = win;
    m_is_showing = true;

    m_window->show_popup(shared_from_this());
}

void PopupWindow::show_at_location(Window* window, int x, int y, Gravity gravity) {
    if (!window) return;

    ensure_container_view();

    Size win_size(window->get_width(), window->get_height());
    Rect anchor_dummy(x, y, 0, 0);
    Size popup_size = measure_popup_size(window, anchor_dummy);

    int target_x = x;
    int target_y = y;

    if (gravity & Gravity::CenterHorizontal) {
        target_x = (window->get_width() - popup_size.width) / 2 + x;
    } else if (gravity & Gravity::Right) {
        target_x = window->get_width() - popup_size.width - x;
    }

    if (gravity & Gravity::CenterVertical) {
        target_y = (window->get_height() - popup_size.height) / 2 + y;
    } else if (gravity & Gravity::Bottom) {
        target_y = window->get_height() - popup_size.height - y;
    }

    Rect calculated(target_x, target_y, popup_size.width, popup_size.height);
    if (m_auto_clamp) {
        calculated = clamp_to_viewport(calculated, win_size);
    }

    m_bounds = calculated;
    m_window = window;
    m_is_showing = true;

    m_window->show_popup(shared_from_this());
}

void PopupWindow::dismiss() {
    if (!m_is_showing) return;
    m_is_showing = false;
    if (m_container_view) {
        if (auto* c = dynamic_cast<PopupContainerView*>(m_container_view.get())) {
            c->clear_views();
        }
    }
    if (m_window) {
        Window* win = m_window;
        m_window = nullptr;
        win->dismiss_popup();
    }
    if (m_on_dismiss) {
        auto cb = std::move(m_on_dismiss);
        m_on_dismiss = nullptr;
        cb();
    }
}

void PopupWindow::notify_dismissed() {
    if (!m_is_showing) return;
    m_is_showing = false;
    m_window = nullptr;
    if (m_container_view) {
        if (auto* c = dynamic_cast<PopupContainerView*>(m_container_view.get())) {
            c->clear_views();
        }
    }
    if (m_on_dismiss) {
        auto cb = std::move(m_on_dismiss);
        m_on_dismiss = nullptr;
        cb();
    }
}

// =============================================================================
// PopupWindowBuilder Implementation
// =============================================================================

PopupWindowBuilder::PopupWindowBuilder()
    : m_popup(std::make_shared<PopupWindow>()) {}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::create() {
    return std::make_shared<PopupWindowBuilder>();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::content(std::shared_ptr<View> view) {
    m_popup->set_content_view(std::move(view));
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::size(int width, int height) {
    m_popup->set_size(width, height);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::width(int width) {
    m_popup->set_width(width);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::height(int height) {
    m_popup->set_height(height);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::maxWidth(int max_w) {
    m_popup->set_max_width(max_w);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::maxHeight(int max_h) {
    m_popup->set_max_height(max_h);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::elevation(int elev) {
    m_popup->set_elevation(elev);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::cornerRadius(int radius) {
    m_popup->set_corner_radius(radius);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::backgroundColor(const Color& color) {
    m_popup->set_background_color(color);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::border(double stroke_width, const Color& stroke_color) {
    m_popup->set_border(stroke_width, stroke_color);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::padding(const Padding& p) {
    m_popup->set_padding(p);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::padding(int uniform) {
    m_popup->set_padding(uniform);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::padding(int h, int v) {
    m_popup->set_padding(h, v);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::padding(int l, int t, int r, int b) {
    m_popup->set_padding(l, t, r, b);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::dismissOnOutsideClick(bool dismiss) {
    m_popup->set_dismiss_on_outside_click(dismiss);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::dismissOnEscape(bool dismiss) {
    m_popup->set_dismiss_on_escape(dismiss);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::autoFlip(bool flip) {
    m_popup->set_auto_flip(flip);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::autoClamp(bool clamp) {
    m_popup->set_auto_clamp(clamp);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::windowMargin(int margin) {
    m_popup->set_window_margin(margin);
    return shared_from_this();
}

std::shared_ptr<PopupWindowBuilder> PopupWindowBuilder::onDismiss(std::function<void()> listener) {
    m_popup->set_on_dismiss_listener(std::move(listener));
    return shared_from_this();
}

std::shared_ptr<PopupWindow> PopupWindowBuilder::build() {
    return m_popup;
}

} // namespace miqu
