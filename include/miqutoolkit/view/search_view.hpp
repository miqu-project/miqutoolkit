#pragma once

#include "miqutoolkit/view/view_group.hpp"
#include "miqutoolkit/view/text_view.hpp"
#include "miqutoolkit/view/edit_text.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <string>
#include <functional>

namespace miqu {

class SearchView : public ViewGroup {
public:
    SearchView();

    void set_title(std::string title);
    const std::string& get_title() const { return m_title; }

    void set_hint(std::string hint);
    const std::string& get_hint() const;

    void set_query(std::string query);
    const std::string& get_query() const;

    void set_focused(bool focus);
    bool is_focused() const;

    void set_radius(int radius) { m_corner_radius = radius; }
    int get_radius() const { return m_corner_radius; }

    void set_background_color(const Color& bg) { m_bg_color = bg; m_has_custom_bg = true; }
    void set_stroke(int width, const Color& stroke) { m_stroke_width = width; m_stroke_color = stroke; }

    void set_on_query_text_listener(std::function<void(const std::string&)> on_change,
                                    std::function<void(const std::string&)> on_submit = nullptr);

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_key(const KeyPressEvent& event) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;

    std::shared_ptr<EditText> get_edit_text() const { return m_edit_text; }
    std::shared_ptr<TextView> get_title_view() const { return m_title_view; }

private:
    std::string m_title;
    std::shared_ptr<TextView> m_title_view;
    std::shared_ptr<EditText> m_edit_text;

    int m_corner_radius = -1;
    int m_stroke_width = 0;
    Color m_stroke_color = Color::transparent();
    Color m_bg_color;
    bool m_has_custom_bg = false;

    std::function<void(const std::string&)> m_on_text_change;
    std::function<void(const std::string&)> m_on_text_submit;
};

class SearchViewBuilder : public std::enable_shared_from_this<SearchViewBuilder> {
public:
    SearchViewBuilder() : m_view(std::make_shared<SearchView>()) {}

    static std::shared_ptr<SearchViewBuilder> create() {
        return std::make_shared<SearchViewBuilder>();
    }

    std::shared_ptr<SearchViewBuilder> title(std::string t) {
        m_view->set_title(std::move(t));
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> hint(std::string h) {
        m_view->set_hint(std::move(h));
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> query(std::string q) {
        m_view->set_query(std::move(q));
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> focused(bool f = true) {
        m_view->set_focused(f);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> cornerRadius(int radius) {
        m_view->set_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> backgroundColor(const Color& bg) {
        m_view->set_background_color(bg);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> stroke(int width, const Color& stroke) {
        m_view->set_stroke(width, stroke);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> onQueryTextListener(std::function<void(const std::string&)> on_change,
                                                          std::function<void(const std::string&)> on_submit = nullptr) {
        m_view->set_on_query_text_listener(std::move(on_change), std::move(on_submit));
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SearchViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<SearchView> build() {
        return m_view;
    }

private:
    std::shared_ptr<SearchView> m_view;
};

} // namespace miqu
