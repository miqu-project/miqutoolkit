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

    void set_text(std::string text) { m_text = std::move(text); }
    const std::string& get_text() const { return m_text; }

    void set_icon(std::string icon) { m_icon = std::move(icon); }
    void set_font_family(std::string family) { m_font_family = std::move(family); }
    const std::string& get_font_family() const { return m_font_family; }
    void set_text_size(int size) { m_font_size = size; }
    void set_bold(bool bold) { m_font_bold = bold; }
    void set_radius(int radius) { m_corner_radius = radius; }

    void set_style(ButtonStyle style) { m_style = style; }
    ButtonStyle get_style() const { return m_style; }
    void set_primary(bool primary = true) { m_style = primary ? ButtonStyle::Primary : ButtonStyle::Standard; }
    void set_outlined(bool outlined = true) { m_style = outlined ? ButtonStyle::Outlined : ButtonStyle::Standard; }
    void set_flat(bool flat = true) { m_style = flat ? ButtonStyle::Flat : ButtonStyle::Standard; }

    void set_custom_colors(const Color& bg, const Color& fg) {
        m_custom_bg = bg;
        m_custom_fg = fg;
        m_use_custom_colors = true;
    }

    void set_selected(bool sel) { m_selected = sel; }
    bool is_selected() const { return m_selected; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    std::string m_text;
    std::string m_icon;
    std::string m_font_family = "";
    int m_font_size = -1;
    bool m_font_bold = false;
    int m_corner_radius = -1;
    ButtonStyle m_style = ButtonStyle::Standard;

    bool m_hovered = false;
    bool m_pressed = false;
    bool m_selected = false;

    bool m_use_custom_colors = false;
    Color m_custom_bg;
    Color m_custom_fg;
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
