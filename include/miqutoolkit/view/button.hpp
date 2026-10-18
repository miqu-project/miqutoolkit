#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>
#include <functional>

namespace miqu {

enum class ButtonStyle {
    Standard, // Surface-variant container with subtle outline
    Primary,  // Accent primary background with on-primary text
    Outlined, // Transparent background with outline border
    Flat      // Borderless, transparent until hovered
};

class Button : public View {
public:
    Button() = default;
    explicit Button(std::string text) : m_text(std::move(text)) {}

    void set_text(std::string text) {
        if (m_text != text) {
            m_text = std::move(text);
            request_redraw();
        }
    }
    const std::string& get_text() const { return m_text; }

    void set_icon(std::string icon) {
        if (m_icon != icon) {
            m_icon = std::move(icon);
            request_redraw();
        }
    }
    void set_font_family(std::string family) {
        if (m_font_family != family) {
            m_font_family = std::move(family);
            request_redraw();
        }
    }
    const std::string& get_font_family() const { return m_font_family; }
    void set_text_size(int size) {
        if (m_font_size != size) {
            m_font_size = size;
            request_redraw();
        }
    }
    void set_bold(bool bold) {
        if (m_font_bold != bold) {
            m_font_bold = bold;
            request_redraw();
        }
    }
    void set_radius(int radius) {
        if (m_corner_radius != radius) {
            m_corner_radius = radius;
            request_redraw();
        }
    }

    void set_style(ButtonStyle style) {
        if (m_style != style) {
            m_style = style;
            request_redraw();
        }
    }
    ButtonStyle get_style() const { return m_style; }
    void set_primary(bool primary = true) { set_style(primary ? ButtonStyle::Primary : ButtonStyle::Standard); }
    void set_outlined(bool outlined = true) { set_style(outlined ? ButtonStyle::Outlined : ButtonStyle::Standard); }
    void set_flat(bool flat = true) { set_style(flat ? ButtonStyle::Flat : ButtonStyle::Standard); }

    void set_icon_size(int size) {
        if (m_icon_size != size) {
            m_icon_size = size;
            request_redraw();
        }
    }
    int get_icon_size() const { return m_icon_size; }

    void set_text_alignment(TextAlignment align) {
        if (m_text_alignment != align) {
            m_text_alignment = align;
            request_redraw();
        }
    }
    TextAlignment get_text_alignment() const { return m_text_alignment; }

    void set_content_gravity(Gravity gravity) {
        if (m_content_gravity != gravity) {
            m_content_gravity = gravity;
            request_redraw();
        }
    }
    Gravity get_content_gravity() const { return m_content_gravity; }

    void set_border_width(double width) {
        if (m_border_width != width) {
            m_border_width = width;
            request_redraw();
        }
    }
    double get_border_width() const { return m_border_width; }
    void set_bordered(bool bordered) { set_border_width(bordered ? -1.0 : 0.0); }
    bool is_bordered() const { return m_border_width != 0.0; }

    void set_custom_colors(const Color& bg, const Color& fg) {
        m_custom_bg = bg;
        m_custom_fg = fg;
        m_use_custom_colors = true;
        request_redraw();
    }

    void set_text_color(const Color& color) {
        m_custom_fg = color;
        m_has_custom_text_color = true;
        request_redraw();
    }

    void set_background_color(const Color& color) {
        m_custom_bg = color;
        m_has_custom_bg = true;
        request_redraw();
    }

    void set_border_color(const Color& color) {
        m_custom_border_color = color;
        m_has_custom_border_color = true;
        request_redraw();
    }

    void set_selected(bool sel) {
        if (m_selected != sel) {
            m_selected = sel;
            request_redraw();
        }
    }
    bool is_selected() const { return m_selected; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    std::string m_text;
    std::string m_icon;
    int m_icon_size = -1;
    std::string m_font_family = "";
    int m_font_size = -1;
    bool m_font_bold = false;
    int m_corner_radius = -1;
    ButtonStyle m_style = ButtonStyle::Standard;
    TextAlignment m_text_alignment = TextAlignment::Center;

    bool m_hovered = false;
    bool m_pressed = false;
    bool m_selected = false;

    bool m_use_custom_colors = false;
    bool m_has_custom_text_color = false;
    bool m_has_custom_bg = false;
    bool m_has_custom_border_color = false;
    Color m_custom_bg;
    Color m_custom_fg;
    Color m_custom_border_color;

    Gravity m_content_gravity = Gravity::Center;
    double m_border_width = -1.0;
};

class ButtonBuilder : public std::enable_shared_from_this<ButtonBuilder> {
public:
    ButtonBuilder() : m_view(std::make_shared<Button>()) {}

    static std::shared_ptr<ButtonBuilder> create() {
        return std::make_shared<ButtonBuilder>();
    }

    std::shared_ptr<ButtonBuilder> text(std::string text) {
        m_view->set_text(std::move(text));
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> icon(std::string icon) {
        m_view->set_icon(std::move(icon));
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> iconSize(int size) {
        m_view->set_icon_size(size);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> fontFamily(std::string family) {
        m_view->set_font_family(std::move(family));
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> textSize(int size) {
        m_view->set_text_size(size);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> bold(bool b = true) {
        m_view->set_bold(b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> cornerRadius(int radius) {
        m_view->set_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> style(ButtonStyle s) {
        m_view->set_style(s);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> primary(bool b = true) {
        m_view->set_primary(b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> outlined(bool b = true) {
        m_view->set_outlined(b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> flat(bool b = true) {
        m_view->set_flat(b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> customColors(const Color& bg, const Color& fg) {
        m_view->set_custom_colors(bg, fg);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> textColor(const Color& fg) {
        m_view->set_text_color(fg);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> backgroundColor(const Color& bg) {
        m_view->set_background_color(bg);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> borderColor(const Color& col) {
        m_view->set_border_color(col);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> textAlignment(TextAlignment a) {
        m_view->set_text_alignment(a);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> contentGravity(Gravity g) {
        m_view->set_content_gravity(g);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> borderWidth(double w) {
        m_view->set_border_width(w);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> bordered(bool b) {
        m_view->set_bordered(b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ButtonBuilder> onClick(std::function<void()> cb) {
        m_view->set_on_click_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<Button> build() {
        return m_view;
    }

private:
    std::shared_ptr<Button> m_view;
};

} // namespace miqu
