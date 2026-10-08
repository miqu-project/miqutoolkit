#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>
#include <functional>

namespace miqu {

class ImageButton : public View {
public:
    ImageButton() = default;
    explicit ImageButton(std::string icon) : m_icon(std::move(icon)) {}

    void set_icon(std::string icon) { m_icon = std::move(icon); }
    const std::string& get_icon() const { return m_icon; }

    void set_image_resource(std::string res) { m_image_resource = std::move(res); }
    const std::string& get_image_resource() const { return m_image_resource; }

    void set_font_family(std::string font) { m_font_family = std::move(font); }
    const std::string& get_font_family() const { return m_font_family; }

    void set_icon_size(int size) { m_icon_size = size; }
    int get_icon_size() const { return m_icon_size; }

    void set_icon_color(const Color& color) { m_icon_color = color; m_custom_icon_color = true; }
    void set_background_color(const Color& color) { m_bg_color = color; m_custom_bg = true; }
    void set_hover_color(const Color& color) { m_hover_color = color; m_custom_hover = true; }

    void set_circle(bool circle) { m_circle = circle; }
    bool is_circle() const { return m_circle; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    std::string m_icon;
    std::string m_image_resource;
    std::string m_font_family;
    int m_icon_size = 18;
    int m_corner_radius = 8;
    bool m_circle = true;

    bool m_custom_icon_color = false;
    Color m_icon_color;

    bool m_custom_bg = false;
    Color m_bg_color = Color::transparent();

    bool m_custom_hover = false;
    Color m_hover_color;

    bool m_hovered = false;
    bool m_pressed = false;
};

class ImageButtonBuilder : public std::enable_shared_from_this<ImageButtonBuilder> {
public:
    ImageButtonBuilder() : m_view(std::make_shared<ImageButton>()) {}

    static std::shared_ptr<ImageButtonBuilder> create() {
        return std::make_shared<ImageButtonBuilder>();
    }

    std::shared_ptr<ImageButtonBuilder> icon(std::string icon) {
        m_view->set_icon(std::move(icon));
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> imageResource(std::string res) {
        m_view->set_image_resource(std::move(res));
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> fontFamily(std::string font) {
        m_view->set_font_family(std::move(font));
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> iconSize(int size) {
        m_view->set_icon_size(size);
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> iconColor(const Color& col) {
        m_view->set_icon_color(col);
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> backgroundColor(const Color& col) {
        m_view->set_background_color(col);
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> hoverColor(const Color& col) {
        m_view->set_hover_color(col);
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> circle(bool circle) {
        m_view->set_circle(circle);
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> cornerRadius(int radius) {
        m_view->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<ImageButtonBuilder> onClick(std::function<void()> listener) {
        m_view->set_on_click_listener(std::move(listener));
        return shared_from_this();
    }

    std::shared_ptr<ImageButton> build() {
        return m_view;
    }

private:
    std::shared_ptr<ImageButton> m_view;
};

} // namespace miqu
