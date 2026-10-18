#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>
#include <memory>

namespace miqu {

enum class AvatarShape {
    Circle,
    RoundedSquare
};

class AvatarView : public View {
public:
    AvatarView() = default;
    explicit AvatarView(int size) : m_size(size) {}

    void set_size(int size) {
        if (m_size != size) {
            m_size = size;
            request_redraw();
        }
    }
    int get_size() const { return m_size; }

    void set_text(std::string text) {
        if (m_text != text) {
            m_text = std::move(text);
            request_redraw();
        }
    }
    const std::string& get_text() const { return m_text; }

    void set_shape(AvatarShape shape) {
        if (m_shape != shape) {
            m_shape = shape;
            request_redraw();
        }
    }
    AvatarShape get_shape() const { return m_shape; }

    void set_corner_radius(int radius) {
        if (m_corner_radius != radius) {
            m_corner_radius = radius;
            request_redraw();
        }
    }
    int get_corner_radius() const { return m_corner_radius; }

    void set_background_color(const Color& color) {
        m_bg_color = color;
        m_has_custom_bg = true;
        request_redraw();
    }

    void set_text_color(const Color& color) {
        m_text_color = color;
        m_has_custom_text_color = true;
        request_redraw();
    }

    void set_border_color(const Color& color) {
        m_border_color = color;
        m_has_custom_border_color = true;
        request_redraw();
    }

    void set_border_width(double width) {
        if (m_border_width != width) {
            m_border_width = width;
            request_redraw();
        }
    }
    double get_border_width() const { return m_border_width; }

    void set_selected(bool selected) {
        if (m_selected != selected) {
            m_selected = selected;
            request_redraw();
        }
    }
    bool is_selected() const { return m_selected; }

    void set_image_resource(std::string path) {
        if (m_image_path != path) {
            m_image_path = std::move(path);
            request_redraw();
        }
    }
    const std::string& get_image_resource() const { return m_image_path; }

    void set_font_size(int size) {
        if (m_font_size != size) {
            m_font_size = size;
            request_redraw();
        }
    }
    int get_font_size() const { return m_font_size; }

    void set_bold(bool bold) {
        if (m_bold != bold) {
            m_bold = bold;
            request_redraw();
        }
    }
    bool is_bold() const { return m_bold; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

private:
    int m_size = 48;
    AvatarShape m_shape = AvatarShape::Circle;
    int m_corner_radius = 8;
    std::string m_text;
    std::string m_image_path;
    int m_font_size = -1;
    bool m_bold = true;
    bool m_selected = false;

    bool m_has_custom_bg = false;
    Color m_bg_color;
    bool m_has_custom_text_color = false;
    Color m_text_color;
    bool m_has_custom_border_color = false;
    Color m_border_color;
    double m_border_width = 1.0;
};

class AvatarViewBuilder : public std::enable_shared_from_this<AvatarViewBuilder> {
public:
    AvatarViewBuilder() : m_view(std::make_shared<AvatarView>()) {}

    static std::shared_ptr<AvatarViewBuilder> create() {
        return std::make_shared<AvatarViewBuilder>();
    }

    std::shared_ptr<AvatarViewBuilder> size(int s) {
        m_view->set_size(s);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> text(std::string t) {
        m_view->set_text(std::move(t));
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> image(std::string path) {
        m_view->set_image_resource(std::move(path));
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> shape(AvatarShape s) {
        m_view->set_shape(s);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> circle() {
        m_view->set_shape(AvatarShape::Circle);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> rounded(int radius = 8) {
        m_view->set_shape(AvatarShape::RoundedSquare);
        m_view->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> backgroundColor(const Color& col) {
        m_view->set_background_color(col);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> textColor(const Color& col) {
        m_view->set_text_color(col);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> borderColor(const Color& col) {
        m_view->set_border_color(col);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> borderWidth(double w) {
        m_view->set_border_width(w);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> selected(bool s = true) {
        m_view->set_selected(s);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> textSize(int s) {
        m_view->set_font_size(s);
        return shared_from_this();
    }

    std::shared_ptr<AvatarViewBuilder> bold(bool b = true) {
        m_view->set_bold(b);
        return shared_from_this();
    }

    std::shared_ptr<AvatarView> build() {
        return m_view;
    }

private:
    std::shared_ptr<AvatarView> m_view;
};

} // namespace miqu
