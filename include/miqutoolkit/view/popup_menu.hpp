#pragma once

#include "miqutoolkit/view/popup_window.hpp"
#include "miqutoolkit/core/color.hpp"
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace miqu {

/**
 * @brief Classification of menu items in PopupMenu.
 */
enum class MenuItemType {
    Action,             ///< Standard clickable menu item with optional icon and shortcut
    Checkable,          ///< Toggle checkbox item with checkmark
    Radio,              ///< Mutually exclusive radio item
    Separator,          ///< Horizontal divider line separating groups
    SectionHeader,      ///< Non-clickable category title
    DestructiveAction,  ///< Danger/destructive action styled with error color
};

/**
 * @brief Represents a single row item inside a PopupMenu.
 */
class MenuItem {
public:
    MenuItem() = default;
    MenuItem(std::string title, std::function<void()> on_click);
    MenuItem(std::string title, std::string icon, std::function<void()> on_click);
    MenuItem(std::string title, std::string icon, std::string shortcut, std::function<void()> on_click);

    // Type
    MenuItemType get_type() const { return m_type; }
    void set_type(MenuItemType type) { m_type = type; }

    // Identification
    int get_id() const { return m_id; }
    void set_id(int id) { m_id = id; }

    // Content
    const std::string& get_title() const { return m_title; }
    void set_title(std::string title) { m_title = std::move(title); }

    const std::string& get_icon() const { return m_icon; }
    void set_icon(std::string icon) { m_icon = std::move(icon); }

    const std::string& get_shortcut() const { return m_shortcut; }
    void set_shortcut(std::string shortcut) { m_shortcut = std::move(shortcut); }

    // State
    bool is_enabled() const { return m_enabled; }
    void set_enabled(bool enabled) { m_enabled = enabled; }

    bool is_checked() const { return m_checked; }
    void set_checked(bool checked) { m_checked = checked; }

    int get_group_id() const { return m_group_id; }
    void set_group_id(int group) { m_group_id = group; }

    bool is_actionable() const {
        return m_enabled && m_type != MenuItemType::Separator && m_type != MenuItemType::SectionHeader;
    }

    // Callbacks
    void set_on_click(std::function<void()> cb) { m_on_click = std::move(cb); }
    const std::function<void()>& get_on_click() const { return m_on_click; }

    void set_on_toggled(std::function<void(bool)> cb) { m_on_toggled = std::move(cb); }
    const std::function<void(bool)>& get_on_toggled() const { return m_on_toggled; }

    void trigger();

private:
    MenuItemType m_type = MenuItemType::Action;
    int m_id = -1;
    std::string m_title;
    std::string m_icon;
    std::string m_shortcut;
    bool m_enabled = true;
    bool m_checked = false;
    int m_group_id = 0;
    std::function<void()> m_on_click;
    std::function<void(bool)> m_on_toggled;
};

class PopupMenuBuilder;

/**
 * @brief High-level popup action menu and selection dropdown system.
 * Supports actions, checkables, radio groups, section headers, separators,
 * icons, keyboard navigation, and automatic viewport boundary adjustment.
 */
class PopupMenu : public std::enable_shared_from_this<PopupMenu> {
public:
    PopupMenu();
    virtual ~PopupMenu();

    // Items management
    void add_item(MenuItem item);
    void add_item(std::string title, std::function<void()> on_click);
    void add_item(std::string title, std::string icon, std::function<void()> on_click);
    void add_item(std::string title, std::string icon, std::string shortcut, std::function<void()> on_click);
    void add_checkable_item(std::string title, bool checked, std::function<void(bool)> on_toggled);
    void add_radio_item(std::string title, int group_id, bool checked, std::function<void(bool)> on_toggled);
    void add_destructive_item(std::string title, std::string icon, std::function<void()> on_click);
    void add_separator();
    void add_section_header(std::string title);
    void clear_items();

    const std::vector<MenuItem>& get_items() const { return m_items; }
    size_t get_item_count() const { return m_items.size(); }

    // Selection & Radio
    void set_selected_index(int index);
    int get_selected_index() const { return m_selected_index; }
    void set_on_item_selected(std::function<void(int index, const MenuItem& item)> listener) {
        m_on_item_selected = std::move(listener);
    }

    // Geometry & Sizing
    void set_min_width(int width) { m_min_width = width; }
    int get_min_width() const { return m_min_width; }

    void set_max_visible_items(int count) { m_max_visible_items = count; }
    int get_max_visible_items() const { return m_max_visible_items; }

    void set_item_height(int height) { m_item_height = height; }
    int get_item_height() const { return m_item_height; }

    // Dismissal
    void set_dismiss_on_select(bool dismiss) { m_dismiss_on_select = dismiss; }
    bool is_dismiss_on_select() const { return m_dismiss_on_select; }

    void set_on_dismiss(std::function<void()> listener) { m_on_dismiss = std::move(listener); }

    // Presentation API
    void show_as_dropdown(const std::shared_ptr<View>& anchor,
                          PopupGravity gravity = PopupGravity::BottomStart,
                          int x_offset = 0, int y_offset = 4);
    void show_as_dropdown(View* anchor,
                          PopupGravity gravity = PopupGravity::BottomStart,
                          int x_offset = 0, int y_offset = 4);
    void show_at_location(Window* window, int x, int y);
    void dismiss();
    bool is_showing() const;

    // Internal invocation from MenuView
    void handle_item_triggered(int index);

private:
    void ensure_popup_window(Window* win);

    std::vector<MenuItem> m_items;
    int m_selected_index = -1;
    int m_min_width = 160;
    int m_item_height = 36;
    int m_max_visible_items = 8;
    bool m_dismiss_on_select = true;
    std::function<void(int, const MenuItem&)> m_on_item_selected;
    std::function<void()> m_on_dismiss;

    std::shared_ptr<PopupWindow> m_popup_window;
    std::shared_ptr<View> m_menu_view;
};

using ActionMenu = PopupMenu;

/**
 * @brief Fluent declarative builder for PopupMenu / ActionMenu.
 */
class PopupMenuBuilder : public std::enable_shared_from_this<PopupMenuBuilder> {
public:
    PopupMenuBuilder();
    static std::shared_ptr<PopupMenuBuilder> create();

    std::shared_ptr<PopupMenuBuilder> item(std::string title, std::function<void()> on_click);
    std::shared_ptr<PopupMenuBuilder> item(std::string title, std::string icon, std::function<void()> on_click);
    std::shared_ptr<PopupMenuBuilder> item(std::string title, std::string icon, std::string shortcut, std::function<void()> on_click);
    std::shared_ptr<PopupMenuBuilder> checkable(std::string title, bool checked, std::function<void(bool)> on_toggled);
    std::shared_ptr<PopupMenuBuilder> radio(std::string title, int group_id, bool checked, std::function<void(bool)> on_toggled);
    std::shared_ptr<PopupMenuBuilder> destructive(std::string title, std::string icon, std::function<void()> on_click);
    std::shared_ptr<PopupMenuBuilder> separator();
    std::shared_ptr<PopupMenuBuilder> section(std::string title);

    std::shared_ptr<PopupMenuBuilder> minWidth(int width);
    std::shared_ptr<PopupMenuBuilder> maxVisibleItems(int count);
    std::shared_ptr<PopupMenuBuilder> itemHeight(int height);
    std::shared_ptr<PopupMenuBuilder> dismissOnSelect(bool dismiss);
    std::shared_ptr<PopupMenuBuilder> onDismiss(std::function<void()> listener);
    std::shared_ptr<PopupMenuBuilder> onItemSelected(std::function<void(int, const MenuItem&)> listener);

    std::shared_ptr<PopupMenu> build();

private:
    std::shared_ptr<PopupMenu> m_menu;
};

using ActionMenuBuilder = PopupMenuBuilder;

} // namespace miqu
