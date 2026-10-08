#pragma once

#include "miqutoolkit/view/view_group.hpp"

namespace miqu {

class RelativeLayout : public ViewGroup {
public:
    RelativeLayout() = default;

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;
};

class RelativeLayoutBuilder : public std::enable_shared_from_this<RelativeLayoutBuilder> {
public:
    RelativeLayoutBuilder() : m_layout(std::make_shared<RelativeLayout>()) {}

    static std::shared_ptr<RelativeLayoutBuilder> create() {
        return std::make_shared<RelativeLayoutBuilder>();
    }

    std::shared_ptr<RelativeLayoutBuilder> addView(std::shared_ptr<View> child) {
        m_layout->add_view(std::move(child));
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> addView(std::shared_ptr<View> child, const LayoutParams& params) {
        m_layout->add_view(std::move(child), params);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> padding(const Padding& p) {
        m_layout->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> padding(int uniform) {
        m_layout->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> padding(int h, int v) {
        m_layout->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> padding(int l, int t, int r, int b) {
        m_layout->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> margin(const Margin& m) {
        m_layout->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> margin(int uniform) {
        m_layout->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> margin(int h, int v) {
        m_layout->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayoutBuilder> margin(int l, int t, int r, int b) {
        m_layout->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<RelativeLayout> build() {
        return m_layout;
    }

private:
    std::shared_ptr<RelativeLayout> m_layout;
};

} // namespace miqu
