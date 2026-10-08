#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>
#include <functional>

namespace miqu {

class EditText : public View {
public:
    EditText() = default;

    void set_text(std::string text);
    const std::string& get_text() const { return m_text; }

    void set_hint(std::string hint) { m_hint = std::move(hint); }
    const std::string& get_hint() const { return m_hint; }

    void set_on_text_changed_listener(std::function<void(std::shared_ptr<EditText>, const std::string&)> cb) {
        m_on_text_changed = std::move(cb);
    }

    void set_on_submit_listener(std::function<void(const std::string&)> cb) {
        m_on_submit = std::move(cb);
    }

    void set_password_mode(bool enable) { m_password_mode = enable; }
    bool is_password_mode() const { return m_password_mode; }

    void set_focused(bool focus);
    bool is_focused() const { return m_focused; }

    void set_draw_background(bool draw) { m_draw_background = draw; }
    bool is_draw_background() const { return m_draw_background; }

    void set_text_color(const Color& col) { m_text_color = col; m_has_custom_text_color = true; }
    void set_hint_color(const Color& col) { m_hint_color = col; m_has_custom_hint_color = true; }
    void set_background_color(const Color& col) { m_bg_color = col; m_has_custom_bg_color = true; }

    const Color& get_text_color() const { return m_text_color; }
    const Color& get_hint_color() const { return m_hint_color; }
    const Color& get_background_color() const { return m_bg_color; }

    void clear();

    void set_window(Window* win) override;
    void on_focus_changed(bool focused) override;

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_key(const KeyPressEvent& event) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    std::string m_text;
    std::string m_hint = "";
    int m_cursor_pos = 0;
    bool m_focused = false;
    bool m_draw_background = true;
    bool m_password_mode = false;

    Color m_text_color;
    Color m_hint_color;
    Color m_bg_color;
    bool m_has_custom_text_color = false;
    bool m_has_custom_hint_color = false;
    bool m_has_custom_bg_color = false;

    std::function<void(std::shared_ptr<EditText>, const std::string&)> m_on_text_changed;
    std::function<void(const std::string&)> m_on_submit;
};

class EditTextBuilder : public std::enable_shared_from_this<EditTextBuilder> {
public:
    EditTextBuilder() : m_view(std::make_shared<EditText>()) {}

    static std::shared_ptr<EditTextBuilder> create() {
        return std::make_shared<EditTextBuilder>();
    }

    std::shared_ptr<EditTextBuilder> hint(std::string h) {
        m_view->set_hint(std::move(h));
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> text(std::string t) {
        m_view->set_text(std::move(t));
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> textColor(const Color& col) {
        m_view->set_text_color(col);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> hintColor(const Color& col) {
        m_view->set_hint_color(col);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> backgroundColor(const Color& col) {
        m_view->set_background_color(col);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> onTextChanged(std::function<void(std::shared_ptr<EditText>, const std::string&)> cb) {
        m_view->set_on_text_changed_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> focused(bool f = true) {
        m_view->set_focused(f);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> drawBackground(bool d = true) {
        m_view->set_draw_background(d);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> passwordMode(bool enable = true) {
        m_view->set_password_mode(enable);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> onSubmit(std::function<void(const std::string&)> cb) {
        m_view->set_on_submit_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<EditTextBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<EditText> build() {
        return m_view;
    }

private:
    std::shared_ptr<EditText> m_view;
};

} // namespace miqu
