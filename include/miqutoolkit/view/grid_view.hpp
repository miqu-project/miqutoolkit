#pragma once

#include "miqutoolkit/view/view.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace miqu {

enum class StretchMode {
    None,
    SpacingWidth,
    ColumnWidth,
};

class GridView : public View {
public:
    GridView();
    ~GridView() override = default;

    void set_num_columns(int cols) { m_cols = std::max(1, cols); m_auto_fit = false; }
    void set_auto_fit(int min_column_width = 100) { m_min_col_w = std::max(20, min_column_width); m_auto_fit = true; }
    void set_cell_size(int w, int h) { m_cell_w = w; m_cell_h = h; }
    void set_cell_height(int h) { m_cell_h = h; }
    void set_horizontal_spacing(int sx) { m_space_x = sx; }
    void set_vertical_spacing(int sy) { m_space_y = sy; }
    void set_stretch_mode(StretchMode mode) { m_stretch_mode = mode; }

    using ItemProviderCallback = std::function<std::shared_ptr<View>(size_t index)>;

    // Item management
    void set_items(std::vector<std::shared_ptr<View>> items);
    void add_item(std::shared_ptr<View> item);
    void set_item_provider(size_t total_count, ItemProviderCallback provider);
    void clear_items();
    size_t get_item_count() const {
        if (m_item_provider) return m_virtual_count;
        return m_items.size();
    }
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
    int compute_columns(int bounds_w, int& out_cell_w) const;
    void ensure_visible(int viewport_height, int cols, int row_stride);
    int item_at(int lx, int ly, const Rect& bounds) const;

    bool m_auto_fit = true;
    int m_min_col_w = 100;
    int m_cols = 5;
    int m_cell_w = 100;
    int m_cell_h = 100;
    int m_space_x = 10;
    int m_space_y = 10;
    StretchMode m_stretch_mode = StretchMode::ColumnWidth;

    int m_selected_index = -1;
    int m_hovered_index = -1;
    int32_t m_touch_id = -1;
    double m_touch_start_x = 0.0;
    double m_touch_start_y = 0.0;
    double m_touch_last_y = 0.0;
    bool m_touch_scrolling = false;
    double m_scroll_y = 0.0;
    mutable int m_last_width = 0;
    mutable int m_last_height = 0;
    mutable int m_effective_cols = 5;
    mutable int m_effective_cell_w = 100;

    std::vector<std::shared_ptr<View>> m_items;
    size_t m_virtual_count = 0;
    ItemProviderCallback m_item_provider;
    std::function<void(size_t, std::shared_ptr<View>)> m_on_item_click;
};

class GridViewBuilder : public std::enable_shared_from_this<GridViewBuilder> {
public:
    GridViewBuilder() : m_view(std::make_shared<GridView>()) {}

    static std::shared_ptr<GridViewBuilder> create() {
        return std::make_shared<GridViewBuilder>();
    }

    std::shared_ptr<GridViewBuilder> itemProvider(size_t total_count, GridView::ItemProviderCallback provider) {
        m_view->set_item_provider(total_count, std::move(provider));
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> numColumns(int cols) {
        m_view->set_num_columns(cols);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> autoFit(int min_column_width = 100) {
        m_view->set_auto_fit(min_column_width);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> cellSize(int w, int h) {
        m_view->set_cell_size(w, h);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> cellHeight(int h) {
        m_view->set_cell_height(h);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> spacing(int sx, int sy) {
        m_view->set_horizontal_spacing(sx);
        m_view->set_vertical_spacing(sy);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> stretchMode(StretchMode mode) {
        m_view->set_stretch_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> items(std::vector<std::shared_ptr<View>> items) {
        m_view->set_items(std::move(items));
        return shared_from_this();
    }

    std::shared_ptr<GridViewBuilder> onItemClick(std::function<void(size_t, std::shared_ptr<View>)> cb) {
        m_view->set_on_item_click_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<GridView> build() {
        return m_view;
    }

private:
    std::shared_ptr<GridView> m_view;
};

} // namespace miqu
