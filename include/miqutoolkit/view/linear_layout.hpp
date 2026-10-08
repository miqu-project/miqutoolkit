#pragma once

#include "miqutoolkit/view/view_group.hpp"

namespace miqu {

class LinearLayout : public ViewGroup {
public:
    LinearLayout() = default;
    explicit LinearLayout(Orientation orient) : m_orientation(orient) {}

    void set_orientation(Orientation orient) { m_orientation = orient; }
    Orientation get_orientation() const { return m_orientation; }

    void set_gravity(Gravity g) { m_gravity = g; }
    Gravity get_gravity() const { return m_gravity; }

    void set_divider_spacing(int spacing) { m_spacing = spacing; }
    int get_divider_spacing() const { return m_spacing; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;
    Size measure_size(int avail_width) const override;

private:
    Orientation m_orientation = Orientation::Vertical;
    Gravity m_gravity = Gravity::None;
    int m_spacing = 0;
};

class LinearLayoutBuilder : public std::enable_shared_from_this<LinearLayoutBuilder> {
public:
    LinearLayoutBuilder() : m_layout(std::make_shared<LinearLayout>()) {}

    static std::shared_ptr<LinearLayoutBuilder> create() {
        return std::make_shared<LinearLayoutBuilder>();
    }

    std::shared_ptr<LinearLayoutBuilder> orientation(Orientation o) {
        m_layout->set_orientation(o);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> gravity(Gravity g) {
        m_layout->set_gravity(g);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> spacing(int s) {
        m_layout->set_divider_spacing(s);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> addView(std::shared_ptr<View> child) {
        m_layout->add_view(std::move(child));
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> addView(std::shared_ptr<View> child, const LayoutParams& params) {
        m_layout->add_view(std::move(child), params);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> padding(const Padding& p) {
        m_layout->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> padding(int uniform) {
        m_layout->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> padding(int h, int v) {
        m_layout->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> padding(int l, int t, int r, int b) {
        m_layout->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> margin(const Margin& m) {
        m_layout->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> margin(int uniform) {
        m_layout->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> margin(int h, int v) {
        m_layout->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> margin(int l, int t, int r, int b) {
        m_layout->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> backgroundColor(const Color& color) {
        m_layout->set_background_color(color);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> stroke(int width, const Color& color) {
        m_layout->set_stroke(width, color);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayoutBuilder> cornerRadius(int radius) {
        m_layout->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<LinearLayout> build() {
        return m_layout;
    }

private:
    std::shared_ptr<LinearLayout> m_layout;
};

} // namespace miqu
