#pragma once

#include "miqutoolkit/view/view_group.hpp"

namespace miqu {

class FrameLayout : public ViewGroup {
public:
    FrameLayout() = default;

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;
    Size measure_size(int avail_width) const override;
};

class FrameLayoutBuilder : public std::enable_shared_from_this<FrameLayoutBuilder> {
public:
    FrameLayoutBuilder() : m_layout(std::make_shared<FrameLayout>()) {}

    static std::shared_ptr<FrameLayoutBuilder> create() {
        return std::make_shared<FrameLayoutBuilder>();
    }

    std::shared_ptr<FrameLayoutBuilder> addView(std::shared_ptr<View> child) {
        m_layout->add_view(std::move(child));
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> addView(std::shared_ptr<View> child, const LayoutParams& params) {
        m_layout->add_view(std::move(child), params);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> padding(const Padding& p) {
        m_layout->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> padding(int uniform) {
        m_layout->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> padding(int h, int v) {
        m_layout->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> padding(int l, int t, int r, int b) {
        m_layout->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> margin(const Margin& m) {
        m_layout->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> margin(int uniform) {
        m_layout->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> margin(int h, int v) {
        m_layout->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> margin(int l, int t, int r, int b) {
        m_layout->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> backgroundColor(const Color& color) {
        m_layout->set_background_color(color);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> stroke(int width, const Color& color) {
        m_layout->set_stroke(width, color);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayoutBuilder> cornerRadius(int radius) {
        m_layout->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<FrameLayout> build() {
        return m_layout;
    }

private:
    std::shared_ptr<FrameLayout> m_layout;
};

} // namespace miqu
