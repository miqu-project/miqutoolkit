#pragma once

#include "miqutoolkit/view/view.hpp"
#include "miqutoolkit/view/layout_params.hpp"
#include "miqutoolkit/core/color.hpp"
#include "miqutoolkit/core/geometry.hpp"
#include <memory>
#include <functional>

namespace miqu {

class Window;

/**
 * @brief Multi-axis gravity placement for PopupWindow relative to an anchor view.
 */
enum class PopupGravity {
    BottomStart,   ///< Below anchor, left-aligned with anchor start
    BottomEnd,     ///< Below anchor, right-aligned with anchor end
    BottomCenter,  ///< Below anchor, centered horizontally
    TopStart,      ///< Above anchor, left-aligned with anchor start
    TopEnd,        ///< Above anchor, right-aligned with anchor end
    TopCenter,     ///< Above anchor, centered horizontally
    StartTop,      ///< To left of anchor, aligned with anchor top
    StartBottom,   ///< To left of anchor, aligned with anchor bottom
    StartCenter,   ///< To left of anchor, centered vertically
    EndTop,        ///< To right of anchor, aligned with anchor top
    EndBottom,     ///< To right of anchor, aligned with anchor bottom
    EndCenter,     ///< To right of anchor, centered vertically
    Center,        ///< Centered on anchor view or viewport
};

class PopupWindowBuilder;

/**
 * @brief Generic floating popup container supporting anchor view positioning,
 * multi-axis gravity, smart viewport boundary flipping/clamping, ambient drop-shadow
 * elevation, and outside-click/escape dismissal.
 */
class PopupWindow : public std::enable_shared_from_this<PopupWindow> {
public:
    PopupWindow();
    explicit PopupWindow(std::shared_ptr<View> content);
    PopupWindow(std::shared_ptr<View> content, int width, int height);
    virtual ~PopupWindow();

    // Content View
    void set_content_view(std::shared_ptr<View> content);
    std::shared_ptr<View> get_content_view() const { return m_content_view; }

    // Dimensions
    void set_size(int width, int height);
    void set_width(int width) { m_width = width; }
    void set_height(int height) { m_height = height; }
    int get_width() const { return m_width; }
    int get_height() const { return m_height; }

    void set_max_width(int max_w) { m_max_width = max_w; }
    int get_max_width() const { return m_max_width; }
    void set_max_height(int max_h) { m_max_height = max_h; }
    int get_max_height() const { return m_max_height; }

    // Styling & Appearance
    void set_elevation(int elevation);
    int get_elevation() const { return m_elevation; }

    void set_corner_radius(int radius);
    int get_corner_radius() const { return m_corner_radius; }

    void set_background_color(const Color& color);
    const Color& get_background_color() const { return m_bg_color; }

    void set_border(double stroke_width, const Color& stroke_color);
    double get_stroke_width() const { return m_stroke_width; }
    const Color& get_stroke_color() const { return m_stroke_color; }

    void set_padding(const Padding& padding);
    void set_padding(int uniform);
    void set_padding(int horizontal, int vertical);
    void set_padding(int left, int top, int right, int bottom);
    const Padding& get_padding() const { return m_padding; }

    // Behavior & Dismissal
    void set_dismiss_on_outside_click(bool dismiss) { m_dismiss_on_outside = dismiss; }
    bool is_dismiss_on_outside_click() const { return m_dismiss_on_outside; }

    void set_dismiss_on_escape(bool dismiss) { m_dismiss_on_escape = dismiss; }
    bool is_dismiss_on_escape() const { return m_dismiss_on_escape; }

    void set_auto_flip(bool flip) { m_auto_flip = flip; }
    bool is_auto_flip() const { return m_auto_flip; }

    void set_auto_clamp(bool clamp) { m_auto_clamp = clamp; }
    bool is_auto_clamp() const { return m_auto_clamp; }

    void set_window_margin(int margin) { m_window_margin = margin; }
    int get_window_margin() const { return m_window_margin; }

    void set_on_dismiss_listener(std::function<void()> listener) { m_on_dismiss = std::move(listener); }

    // Presentation API
    void show_as_dropdown(const std::shared_ptr<View>& anchor,
                          PopupGravity gravity = PopupGravity::BottomStart,
                          int x_offset = 0, int y_offset = 0);
    void show_as_dropdown(View* anchor,
                          PopupGravity gravity = PopupGravity::BottomStart,
                          int x_offset = 0, int y_offset = 0);
    void show_at_location(Window* window, int x, int y,
                          Gravity gravity = Gravity::None);
    void dismiss();
    bool is_showing() const { return m_is_showing; }

    // Target geometry in window coordinates
    const Rect& get_bounds() const { return m_bounds; }

    // Internal container view attached to Window overlay
    std::shared_ptr<View> get_container_view() const { return m_container_view; }

    // Invoked by Window when dismissed
    void notify_dismissed();

private:
    Rect calculate_raw_anchor_bounds(const Rect& anchor_rect,
                                     const Size& popup_size,
                                     PopupGravity gravity,
                                     int x_offset, int y_offset) const;
    Rect apply_boundary_flip(const Rect& raw_rect,
                             const Rect& anchor_rect,
                             const Size& popup_size,
                             const Size& window_size,
                             PopupGravity gravity,
                             int x_offset, int y_offset) const;
    Rect clamp_to_viewport(const Rect& rect, const Size& window_size) const;
    Size measure_popup_size(Window* window, const Rect& anchor_rect) const;
    void ensure_container_view();

    std::shared_ptr<View> m_content_view;
    std::shared_ptr<View> m_container_view;
    Window* m_window = nullptr;

    int m_width = static_cast<int>(LayoutDimension::WrapContent);
    int m_height = static_cast<int>(LayoutDimension::WrapContent);
    int m_max_width = 0;
    int m_max_height = 0;

    int m_elevation = 4;
    int m_corner_radius = -1; // -1 denotes default from theme metrics
    Color m_bg_color;
    bool m_has_bg_color = false;
    double m_stroke_width = 1.0;
    Color m_stroke_color;
    bool m_has_stroke_color = false;
    Padding m_padding{0, 0, 0, 0};

    bool m_dismiss_on_outside = true;
    bool m_dismiss_on_escape = true;
    bool m_auto_flip = true;
    bool m_auto_clamp = true;
    int m_window_margin = 8;

    bool m_is_showing = false;
    Rect m_bounds;
    std::function<void()> m_on_dismiss;
};

/**
 * @brief Declarative fluent builder for PopupWindow.
 */
class PopupWindowBuilder : public std::enable_shared_from_this<PopupWindowBuilder> {
public:
    PopupWindowBuilder();

    static std::shared_ptr<PopupWindowBuilder> create();

    std::shared_ptr<PopupWindowBuilder> content(std::shared_ptr<View> view);
    std::shared_ptr<PopupWindowBuilder> size(int width, int height);
    std::shared_ptr<PopupWindowBuilder> width(int width);
    std::shared_ptr<PopupWindowBuilder> height(int height);
    std::shared_ptr<PopupWindowBuilder> maxWidth(int max_w);
    std::shared_ptr<PopupWindowBuilder> maxHeight(int max_h);

    std::shared_ptr<PopupWindowBuilder> elevation(int elev);
    std::shared_ptr<PopupWindowBuilder> cornerRadius(int radius);
    std::shared_ptr<PopupWindowBuilder> backgroundColor(const Color& color);
    std::shared_ptr<PopupWindowBuilder> border(double stroke_width, const Color& stroke_color);
    std::shared_ptr<PopupWindowBuilder> padding(const Padding& p);
    std::shared_ptr<PopupWindowBuilder> padding(int uniform);
    std::shared_ptr<PopupWindowBuilder> padding(int h, int v);
    std::shared_ptr<PopupWindowBuilder> padding(int l, int t, int r, int b);

    std::shared_ptr<PopupWindowBuilder> dismissOnOutsideClick(bool dismiss);
    std::shared_ptr<PopupWindowBuilder> dismissOnEscape(bool dismiss);
    std::shared_ptr<PopupWindowBuilder> autoFlip(bool flip);
    std::shared_ptr<PopupWindowBuilder> autoClamp(bool clamp);
    std::shared_ptr<PopupWindowBuilder> windowMargin(int margin);

    std::shared_ptr<PopupWindowBuilder> onDismiss(std::function<void()> listener);

    std::shared_ptr<PopupWindow> build();

private:
    std::shared_ptr<PopupWindow> m_popup;
};

} // namespace miqu
