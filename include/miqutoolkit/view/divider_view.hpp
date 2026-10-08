#pragma once

#include "miqutoolkit/view/view.hpp"
#include <algorithm>

namespace miqu {

class DividerView : public View {
public:
    DividerView(Orientation orientation = Orientation::Horizontal, int thickness = 1)
        : m_orientation(orientation), m_thickness(std::max(1, thickness)) {}

    void set_orientation(Orientation o) { m_orientation = o; }
    Orientation get_orientation() const { return m_orientation; }

    void set_thickness(int t) { m_thickness = std::max(1, t); }
    int get_thickness() const { return m_thickness; }

    void set_color(const Color& color) { m_color = color; m_has_custom_color = true; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

private:
    Orientation m_orientation = Orientation::Horizontal;
    int m_thickness = 1;
    bool m_has_custom_color = false;
    Color m_color;
};

class DividerViewBuilder : public std::enable_shared_from_this<DividerViewBuilder> {
public:
    DividerViewBuilder() : m_view(std::make_shared<DividerView>()) {}

    static std::shared_ptr<DividerViewBuilder> create() {
        return std::make_shared<DividerViewBuilder>();
    }

    std::shared_ptr<DividerViewBuilder> orientation(Orientation o) {
        m_view->set_orientation(o);
        return shared_from_this();
    }

    std::shared_ptr<DividerViewBuilder> thickness(int t) {
        m_view->set_thickness(t);
        return shared_from_this();
    }

    std::shared_ptr<DividerViewBuilder> color(const Color& c) {
        m_view->set_color(c);
        return shared_from_this();
    }

    std::shared_ptr<DividerViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<DividerViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<DividerViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<DividerViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<DividerView> build() {
        return m_view;
    }

private:
    std::shared_ptr<DividerView> m_view;
};

} // namespace miqu
