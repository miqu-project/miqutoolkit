#pragma once

#include "miqutoolkit/view/frame_layout.hpp"

namespace miqu {

enum class CardStyle {
    Filled,    // Modern borderless surface card (default)
    Outlined,  // Card with outline border
    Elevated   // Elevated card with subtle ambient drop shadow
};

class CardView : public FrameLayout {
public:
    CardView() {
        m_corner_radius = -1; // -1 denotes default from theme metrics
        m_style = CardStyle::Filled;
        m_elevation = 0;
    }

    void set_card_background_color(const Color& color) { set_background_color(color); }
    const Color& get_card_background_color() const { return get_background_color(); }

    void set_radius(int radius) { set_corner_radius(radius); }
    int get_radius() const { return get_corner_radius(); }

    void set_style(CardStyle style) { m_style = style; }
    CardStyle get_style() const { return m_style; }

    void set_borderless(bool borderless = true) {
        m_style = borderless ? CardStyle::Filled : CardStyle::Outlined;
    }
    bool is_borderless() const { return m_style != CardStyle::Outlined && m_stroke_width <= 0; }

    void set_bordered(bool bordered = true) {
        m_style = bordered ? CardStyle::Outlined : CardStyle::Filled;
    }
    bool is_bordered() const { return m_style == CardStyle::Outlined || m_stroke_width > 0; }

    void set_elevation(int elevation) { m_elevation = elevation; }
    int get_elevation() const { return m_elevation; }

    void draw(cairo_t* cr, const Rect& bounds) override;

    using ViewGroup::add_view;
    void add_view(std::shared_ptr<View> child) override {
        if (!child) return;
        if (child->get_layout_params().width == static_cast<int>(LayoutDimension::WrapContent)) {
            auto params = child->get_layout_params();
            params.width = static_cast<int>(LayoutDimension::MatchParent);
            child->set_layout_params(params);
        }
        FrameLayout::add_view(std::move(child));
    }

    static void draw_rounded_rect(cairo_t* cr, double x, double y, double w, double h, double r);

private:
    CardStyle m_style = CardStyle::Filled;
    int m_elevation = 0;
};

class CardViewBuilder : public std::enable_shared_from_this<CardViewBuilder> {
public:
    CardViewBuilder() : m_view(std::make_shared<CardView>()) {}

    static std::shared_ptr<CardViewBuilder> create() {
        return std::make_shared<CardViewBuilder>();
    }

    std::shared_ptr<CardViewBuilder> style(CardStyle s) {
        m_view->set_style(s);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> borderless(bool b = true) {
        m_view->set_borderless(b);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> bordered(bool b = true) {
        m_view->set_bordered(b);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> outlined(bool o = true) {
        m_view->set_bordered(o);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> elevation(int elev) {
        m_view->set_elevation(elev);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> backgroundColor(const Color& col) {
        m_view->set_card_background_color(col);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> stroke(int width, const Color& col) {
        m_view->set_stroke(width, col);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> cornerRadius(int radius) {
        m_view->set_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> addView(std::shared_ptr<View> child) {
        m_view->add_view(std::move(child));
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> addView(std::shared_ptr<View> child, const LayoutParams& params) {
        m_view->add_view(std::move(child), params);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<CardViewBuilder> bounds(const Rect& r) {
        m_view->set_bounds(r);
        return shared_from_this();
    }

    std::shared_ptr<CardView> build() {
        return m_view;
    }

private:
    std::shared_ptr<CardView> m_view;
};

} // namespace miqu
