#pragma once

#include "miqutoolkit/view/view.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace miqu {

class ListView : public View {
public:
    ListView();
    ~ListView() override = default;

    void set_item_height(int h) { m_item_h = std::max(16, h); }
    int get_item_height() const { return m_item_h; }

    void set_spacing(int spacing) { m_spacing = std::max(0, spacing); }
    int get_spacing() const { return m_spacing; }

    void set_item_corner_radius(int radius) { m_corner_radius = radius; }
    int get_item_corner_radius() const { return m_corner_radius; }

    void set_show_dividers(bool show) { m_show_dividers = show; }
    bool get_show_dividers() const { return m_show_dividers; }

    // Item management
    void set_items(std::vector<std::shared_ptr<View>> items);
    void add_item(std::shared_ptr<View> item);
    void clear_items();
    size_t get_item_count() const { return m_items.size(); }
    std::shared_ptr<View> get_item_at(size_t index) const;

    // Selection
    void set_selected_index(int index);
    int get_selected_index() const { return m_selected_index; }
    std::shared_ptr<View> get_selected_item() const;

    // Event listener
    void set_on_item_click_listener(std::function<void(size_t index, std::shared_ptr<View> item)> cb) {
        m_on_item_click = std::move(cb);
    }

    void draw(cairo_t* cr, const Rect& bounds) override;
    bool on_key(const KeyPressEvent& event) override;
    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_scroll(double delta) override;
    bool on_touch(const TouchEvent& event, const Rect& bounds) override;

private:
    void ensure_visible(int viewport_height);
    int item_at(int lx, int ly, const Rect& bounds) const;

    int m_item_h = 44;
    int m_spacing = 2;
    int m_corner_radius = 6;
    bool m_show_dividers = false;

    int m_selected_index = -1;
    int m_hovered_index = -1;

    int32_t m_touch_id = -1;
    double m_touch_start_x = 0.0;
    double m_touch_start_y = 0.0;
    double m_touch_last_y = 0.0;
    bool m_touch_scrolling = false;
    double m_scroll_y = 0.0;
    mutable int m_last_height = 0;

    std::vector<std::shared_ptr<View>> m_items;
    std::function<void(size_t, std::shared_ptr<View>)> m_on_item_click;
};

class ListViewBuilder : public std::enable_shared_from_this<ListViewBuilder> {
public:
    static std::shared_ptr<ListViewBuilder> create() {
        return std::make_shared<ListViewBuilder>();
    }

    ListViewBuilder() : m_view(std::make_shared<ListView>()) {}

    std::shared_ptr<ListViewBuilder> itemHeight(int h) {
        m_view->set_item_height(h);
        return shared_from_this();
    }

    std::shared_ptr<ListViewBuilder> spacing(int spacing) {
        m_view->set_spacing(spacing);
        return shared_from_this();
    }

    std::shared_ptr<ListViewBuilder> itemCornerRadius(int r) {
        m_view->set_item_corner_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<ListViewBuilder> showDividers(bool show) {
        m_view->set_show_dividers(show);
        return shared_from_this();
    }

    std::shared_ptr<ListViewBuilder> items(std::vector<std::shared_ptr<View>> items) {
        m_view->set_items(std::move(items));
        return shared_from_this();
    }

    std::shared_ptr<ListViewBuilder> onItemClick(std::function<void(size_t, std::shared_ptr<View>)> cb) {
        m_view->set_on_item_click_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<ListView> build() {
        return m_view;
    }

private:
    std::shared_ptr<ListView> m_view;
};

} // namespace miqu
