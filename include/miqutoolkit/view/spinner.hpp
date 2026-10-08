#pragma once

#include "miqutoolkit/view/view.hpp"
#include <vector>
#include <string>
#include <functional>

namespace miqu {

class PopupMenu;

class Spinner : public View {
public:
    Spinner();
    ~Spinner() override;

    // Items list
    void set_items(std::vector<std::string> items);
    void add_item(std::string item);
    void clear_items();
    const std::vector<std::string>& get_items() const { return m_items; }
    size_t get_item_count() const { return m_items.size(); }

    // Selection
    void set_selected_index(int index);
    int get_selected_index() const { return m_selected_index; }
    std::string get_selected_item() const;

    // Prompt / Placeholder
    void set_prompt(std::string prompt) { m_prompt = std::move(prompt); request_redraw(); }
    const std::string& get_prompt() const { return m_prompt; }

    // Typography
    void set_font_family(std::string family) { m_font_family = std::move(family); request_redraw(); }
    const std::string& get_font_family() const { return m_font_family; }

    void set_font_size(int size) { m_font_size = size; request_redraw(); }
    int get_font_size() const { return m_font_size; }
    void set_text_size(int size) { set_font_size(size); }
    int get_text_size() const { return get_font_size(); }

    // Corner Radius & Border
    void set_corner_radius(int radius) { m_corner_radius = radius; request_redraw(); }
    int get_corner_radius() const { return m_corner_radius; }
    void set_radius(int radius) { set_corner_radius(radius); }
    int get_radius() const { return get_corner_radius(); }

    // Colors
    void set_background_color(const Color& col) { m_bg_color = col; m_has_custom_bg = true; request_redraw(); }
    const Color& get_background_color() const { return m_bg_color; }
    bool has_custom_bg() const { return m_has_custom_bg; }

    void set_text_color(const Color& col) { m_text_color = col; m_has_custom_color = true; request_redraw(); }
    const Color& get_text_color() const { return m_text_color; }
    bool has_custom_color() const { return m_has_custom_color; }

    // Popup Layout Config
    void set_max_visible_items(int count) { m_max_visible_items = std::max(1, count); }
    int get_max_visible_items() const { return m_max_visible_items; }

    void set_item_height(int h) { m_item_height = std::max(16, h); }
    int get_item_height() const { return m_item_height; }

    // Callbacks
    void set_on_item_selected_listener(std::function<void(int index, const std::string& item)> listener) {
        m_on_item_selected = std::move(listener);
    }

    // Dropdown Lifecycle
    void open_dropdown();
    void close_dropdown();
    bool is_open() const { return m_is_open; }

    // View Lifecycle & Event Overrides
    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;
    Size measure_size(int avail_width) const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_key(const KeyPressEvent& event) override;

    // Internal notification when popup item is selected or closed
    void handle_popup_selected(int index);
    void handle_popup_dismissed();

private:
    std::vector<std::string> m_items;
    int m_selected_index = -1;
    std::string m_prompt;
    std::string m_font_family = "";
    int m_font_size = -1;
    int m_corner_radius = -1;
    int m_item_height = 36;
    int m_max_visible_items = 6;

    Color m_bg_color;
    bool m_has_custom_bg = false;
    Color m_text_color;
    bool m_has_custom_color = false;

    bool m_hovered = false;
    bool m_is_open = false;
    Rect m_last_drawn_bounds;

    std::shared_ptr<PopupMenu> m_popup_menu;
    std::function<void(int, const std::string&)> m_on_item_selected;
};

class SpinnerBuilder : public std::enable_shared_from_this<SpinnerBuilder> {
public:
    SpinnerBuilder() : m_view(std::make_shared<Spinner>()) {}

    static std::shared_ptr<SpinnerBuilder> create() {
        return std::make_shared<SpinnerBuilder>();
    }

    std::shared_ptr<SpinnerBuilder> items(std::vector<std::string> items) {
        m_view->set_items(std::move(items));
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> addItem(std::string item) {
        m_view->add_item(std::move(item));
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> selectedIndex(int index) {
        m_view->set_selected_index(index);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> prompt(std::string prompt) {
        m_view->set_prompt(std::move(prompt));
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> fontFamily(std::string family) {
        m_view->set_font_family(std::move(family));
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> fontSize(int size) {
        m_view->set_font_size(size);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> textSize(int size) {
        m_view->set_text_size(size);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> cornerRadius(int radius) {
        m_view->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> backgroundColor(const Color& col) {
        m_view->set_background_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> textColor(const Color& col) {
        m_view->set_text_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> maxVisibleItems(int count) {
        m_view->set_max_visible_items(count);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> itemHeight(int h) {
        m_view->set_item_height(h);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> onItemSelected(std::function<void(int, const std::string&)> listener) {
        m_view->set_on_item_selected_listener(std::move(listener));
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SpinnerBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<Spinner> build() {
        return m_view;
    }

private:
    std::shared_ptr<Spinner> m_view;
};

} // namespace miqu
