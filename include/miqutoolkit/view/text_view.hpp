#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>

namespace miqu {

enum class TextAlignment {
    Left,
    Center,
    Right,
    Justify
};

enum class TextVerticalAlignment {
    Top,
    Center,
    Bottom
};

enum class WrapMode {
    None,
    Word,
    Char,
    WordChar
};

enum class EllipsizeMode {
    None,
    Start,
    Middle,
    End
};

class TextView : public View {
public:
    TextView() = default;
    explicit TextView(std::string text) : m_text(std::move(text)) {}

    void set_text(std::string text) {
        if (m_text != text) {
            m_text = std::move(text);
            request_redraw();
        }
    }
    const std::string& get_text() const { return m_text; }

    void set_text_color(const Color& color) { m_color = color; m_has_custom_color = true; request_redraw(); }
    const Color& get_text_color() const { return m_color; }
    bool has_custom_color() const { return m_has_custom_color; }

    void set_font_family(std::string family) { m_font_family = std::move(family); request_redraw(); }
    const std::string& get_font_family() const { return m_font_family; }

    void set_text_size(int size) { m_font_size = size; request_redraw(); }
    int get_text_size() const { return m_font_size; }

    void set_bold(bool bold) { m_bold = bold; request_redraw(); }
    bool is_bold() const { return m_bold; }

    void set_italic(bool italic) { m_italic = italic; request_redraw(); }
    bool is_italic() const { return m_italic; }

    void set_heading_level(int level) { m_heading_level = level; request_redraw(); }
    int get_heading_level() const { return m_heading_level; }

    void set_caption(bool c) { m_caption = c; }
    bool is_caption() const { return m_caption; }

    void set_muted(bool m) { m_muted = m; }
    bool is_muted() const { return m_muted; }

    void set_text_alignment(TextAlignment align) { m_align = align; }
    TextAlignment get_text_alignment() const { return m_align; }

    void set_vertical_alignment(TextVerticalAlignment align) {
        m_vertical_align = align;
        m_has_custom_vertical_align = true;
    }
    TextVerticalAlignment get_vertical_alignment() const { return m_vertical_align; }

    void set_multiline(bool m) {
        m_multiline = m;
        if (m && m_wrap_mode == WrapMode::None) {
            m_wrap_mode = WrapMode::WordChar;
        }
    }
    bool is_multiline() const { return m_multiline; }

    void set_wrap(bool w) { set_multiline(w); }
    bool is_wrap() const { return m_multiline; }

    void set_wrap_mode(WrapMode mode) {
        m_wrap_mode = mode;
        if (mode != WrapMode::None) {
            m_multiline = true;
        }
    }
    WrapMode get_wrap_mode() const { return m_wrap_mode; }

    void set_max_lines(int max_lines) { m_max_lines = max_lines; }
    int get_max_lines() const { return m_max_lines; }

    void set_ellipsize(bool ellipsize) {
        m_ellipsize = ellipsize;
        if (!ellipsize) {
            m_ellipsize_mode = EllipsizeMode::None;
        } else if (m_ellipsize_mode == EllipsizeMode::None) {
            m_ellipsize_mode = EllipsizeMode::End;
        }
    }
    bool is_ellipsize() const { return m_ellipsize && m_ellipsize_mode != EllipsizeMode::None; }

    void set_ellipsize_mode(EllipsizeMode mode) {
        m_ellipsize_mode = mode;
        m_ellipsize = (mode != EllipsizeMode::None);
    }
    EllipsizeMode get_ellipsize_mode() const { return m_ellipsize_mode; }

    void set_line_spacing(float spacing) { m_line_spacing = spacing; }
    float get_line_spacing() const { return m_line_spacing; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;
    Size measure_size(int avail_width) const override;

private:
    std::string m_text;
    std::string m_font_family = "";
    Color m_color;
    bool m_has_custom_color = false;
    int m_font_size = -1;
    bool m_bold = false;
    bool m_italic = false;
    int m_heading_level = 0; // 0 = normal, 1 = h1, 2 = h2, 3 = h3
    bool m_caption = false;
    bool m_muted = false;
    TextAlignment m_align = TextAlignment::Left;
    TextVerticalAlignment m_vertical_align = TextVerticalAlignment::Center;
    bool m_has_custom_vertical_align = false;
    bool m_multiline = false;
    WrapMode m_wrap_mode = WrapMode::None;
    int m_max_lines = -1; // <= 0 means unlimited
    bool m_ellipsize = true;
    EllipsizeMode m_ellipsize_mode = EllipsizeMode::End;
    float m_line_spacing = 1.0f;
};

class TextViewBuilder : public std::enable_shared_from_this<TextViewBuilder> {
public:
    TextViewBuilder() : m_view(std::make_shared<TextView>()) {}

    static std::shared_ptr<TextViewBuilder> create() {
        return std::make_shared<TextViewBuilder>();
    }

    std::shared_ptr<TextViewBuilder> text(std::string text) {
        m_view->set_text(std::move(text));
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> textColor(const Color& col) {
        m_view->set_text_color(col);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> fontFamily(std::string family) {
        m_view->set_font_family(std::move(family));
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> textSize(int size) {
        m_view->set_text_size(size);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> h1() {
        m_view->set_heading_level(1);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> h2() {
        m_view->set_heading_level(2);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> h3() {
        m_view->set_heading_level(3);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> caption(bool c = true) {
        m_view->set_caption(c);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> muted(bool m = true) {
        m_view->set_muted(m);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> bold(bool b = true) {
        m_view->set_bold(b);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> textAlignment(TextAlignment a) {
        m_view->set_text_alignment(a);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> ellipsize(bool e = true) {
        m_view->set_ellipsize(e);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> ellipsizeMode(EllipsizeMode mode) {
        m_view->set_ellipsize_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> multiline(bool m = true) {
        m_view->set_multiline(m);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> wrap(bool w = true) {
        m_view->set_wrap(w);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> wrapMode(WrapMode mode) {
        m_view->set_wrap_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> maxLines(int lines) {
        m_view->set_max_lines(lines);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> verticalAlignment(TextVerticalAlignment a) {
        m_view->set_vertical_alignment(a);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> lineSpacing(float s) {
        m_view->set_line_spacing(s);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<TextViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<TextView> build() {
        return m_view;
    }

private:
    std::shared_ptr<TextView> m_view;
};

} // namespace miqu
