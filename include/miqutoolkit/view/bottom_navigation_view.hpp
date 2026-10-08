#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>
#include <vector>
#include <functional>

namespace miqu {

struct BottomNavItem {
    std::string label;
    std::string icon;
    std::string badge; // Optional: e.g. "3" or ""

    BottomNavItem() = default;
    BottomNavItem(std::string l, std::string i, std::string b = "")
        : label(std::move(l)), icon(std::move(i)), badge(std::move(b)) {}
};

class BottomNavigationView : public View {
public:
    BottomNavigationView();

    void set_items(const std::vector<BottomNavItem>& items);
    void add_item(const BottomNavItem& item);
    void add_item(const std::string& label, const std::string& icon, const std::string& badge = "");

    const std::vector<BottomNavItem>& get_items() const { return m_items; }
    size_t get_item_count() const { return m_items.size(); }

    void set_selected_index(int index);
    int get_selected_index() const { return m_selected_index; }

    void set_on_item_selected_listener(std::function<void(int index)> cb) {
        m_on_item_selected = std::move(cb);
    }

    void set_show_divider(bool show) { m_show_divider = show; }
    bool is_show_divider() const { return m_show_divider; }

    void set_bar_height(int h) { m_bar_height = h; }
    int get_bar_height() const { return m_bar_height; }

    void set_pill_size(int w, int h) { m_pill_w = w; m_pill_h = h; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    void set_item_width(int w) { m_item_width = w; }
    int get_item_width() const { return m_item_width; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    std::vector<BottomNavItem> m_items;
    int m_selected_index = 0;
    int m_hovered_index = -1;
    int m_pressed_index = -1;
    bool m_show_divider = true;
    int m_corner_radius = 0;
    int m_bar_height = 56;
    int m_item_width = 72;
    int m_pill_w = 54;
    int m_pill_h = 28;

    std::function<void(int)> m_on_item_selected;

    int get_item_index_at(int lx, int ly, const Rect& bounds) const;
};

class BottomNavigationViewBuilder : public std::enable_shared_from_this<BottomNavigationViewBuilder> {
public:
    BottomNavigationViewBuilder();

    static std::shared_ptr<BottomNavigationViewBuilder> create() {
        return std::make_shared<BottomNavigationViewBuilder>();
    }

    std::shared_ptr<BottomNavigationViewBuilder> items(const std::vector<BottomNavItem>& items) {
        m_view->set_items(items);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> addItem(const std::string& label, const std::string& icon, const std::string& badge = "") {
        m_view->add_item(label, icon, badge);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> addItem(const BottomNavItem& item) {
        m_view->add_item(item);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> selectedIndex(int index) {
        m_view->set_selected_index(index);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> onItemSelected(std::function<void(int index)> cb) {
        m_view->set_on_item_selected_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> showDivider(bool show) {
        m_view->set_show_divider(show);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> cornerRadius(int r) {
        m_view->set_corner_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> itemWidth(int w) {
        m_view->set_item_width(w);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> barHeight(int h) {
        m_view->set_bar_height(h);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> pillSize(int w, int h) {
        m_view->set_pill_size(w, h);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<BottomNavigationView> build() {
        return m_view;
    }

private:
    std::shared_ptr<BottomNavigationView> m_view;
};

} // namespace miqu
