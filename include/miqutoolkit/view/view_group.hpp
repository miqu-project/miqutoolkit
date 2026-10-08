#pragma once

#include "miqutoolkit/view/view.hpp"
#include <vector>
#include <memory>
#include <algorithm>

namespace miqu {

class ViewGroup : public View {
public:
    ViewGroup() = default;
    virtual ~ViewGroup() = default;

    virtual void add_view(std::shared_ptr<View> child);
    virtual void add_view(std::shared_ptr<View> child, const LayoutParams& params);
    virtual void remove_view(std::shared_ptr<View> child);
    virtual void clear_views();

    size_t get_child_count() const { return m_children.size(); }
    std::shared_ptr<View> get_child_at(size_t index) const {
        if (index < m_children.size()) return m_children[index];
        return nullptr;
    }

    void set_window(Window* win) override;

    void set_background_color(const Color& color) { m_background_color = color; m_has_custom_bg = true; }
    const Color& get_background_color() const { return m_background_color; }
    bool has_background() const { return m_has_custom_bg && m_background_color.a > 0.0f; }

    void set_stroke(int width, const Color& color) { m_stroke_width = width; m_stroke_color = color; }
    int get_stroke_width() const { return m_stroke_width; }
    const Color& get_stroke_color() const { return m_stroke_color; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    static void draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r);

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_key(const KeyPressEvent& event) override;
    bool on_scroll(double delta) override;
    bool on_touch(const TouchEvent& event, const Rect& bounds) override;

protected:
    void draw_background(cairo_t* cr, const Rect& bounds) const;
    void draw_stroke(cairo_t* cr, const Rect& bounds) const;

    struct ChildEntry {
        std::shared_ptr<View> view;
        Rect allocated_bounds;
    };

    std::vector<std::shared_ptr<View>> m_children;
    std::vector<ChildEntry> m_child_entries;

    Color m_background_color = Color::transparent();
    bool m_has_custom_bg = false;
    int m_stroke_width = 0;
    Color m_stroke_color = Color::transparent();
    int m_corner_radius = 0;
    bool m_pressed = false;

    std::shared_ptr<View> m_touch_target;
    Rect m_touch_target_bounds;
};

} // namespace miqu
