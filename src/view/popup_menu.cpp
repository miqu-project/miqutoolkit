#include "miqutoolkit/view/popup_menu.hpp"
#include "miqutoolkit/view/card_view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include "miqutoolkit/core/config.hpp"
#include "miqutoolkit/core/window.hpp"
#include <pango/pangocairo.h>
#include <xkbcommon/xkbcommon-keysyms.h>
#include <algorithm>
#include <cmath>

namespace miqu {

namespace {

using PangoLayoutPtr = std::unique_ptr<PangoLayout, decltype(&g_object_unref)>;
using PangoFontDescPtr = std::unique_ptr<PangoFontDescription, decltype(&pango_font_description_free)>;

const Color DANGER_COLOR(0.92f, 0.26f, 0.26f, 1.0f);
constexpr int SEPARATOR_HEIGHT = 9;
constexpr int SECTION_HEADER_HEIGHT = 24;

/**
 * @brief Internal view rendering the menu list, keyboard navigation,
 * hover pills, icons, titles, shortcuts, and checkmark indicators.
 */
class PopupMenuView : public View {
public:
    PopupMenuView(std::weak_ptr<PopupMenu> menu,
                  int item_height,
                  int max_visible_items)
        : m_menu(std::move(menu)),
          m_item_height(item_height),
          m_max_visible_items(max_visible_items) {
        set_layout_params(LayoutParams(
            static_cast<int>(LayoutDimension::MatchParent),
            static_cast<int>(LayoutDimension::MatchParent)
        ));

        // Initial hover on the selected item if valid, or first actionable item
        if (auto menu_sp = m_menu.lock()) {
            int sel = menu_sp->get_selected_index();
            if (sel >= 0 && sel < static_cast<int>(menu_sp->get_items().size()) &&
                menu_sp->get_items()[sel].is_actionable()) {
                m_hovered_index = sel;
            } else {
                m_hovered_index = find_next_actionable(-1, 1);
            }
        }
        ensure_visible(m_hovered_index);
    }

    int get_row_height(const MenuItem& item) const {
        if (item.get_type() == MenuItemType::Separator) return SEPARATOR_HEIGHT;
        if (item.get_type() == MenuItemType::SectionHeader) return SECTION_HEADER_HEIGHT;
        return m_item_height;
    }

    int get_total_content_height() const {
        auto menu_sp = m_menu.lock();
        if (!menu_sp) return 0;
        int total = 8; // top and bottom margin
        for (const auto& item : menu_sp->get_items()) {
            total += get_row_height(item);
        }
        return total;
    }

    Size measure_size() const override {
        return measure_size(-1);
    }

    Size measure_size(int) const override {
        auto menu_sp = m_menu.lock();
        if (!menu_sp) return Size(160, 40);

        auto config = Config::get();
        int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 11;
        int max_w = menu_sp->get_min_width();

        int max_title_chars = 0;
        int max_shortcut_chars = 0;
        bool any_icon = false;
        bool any_check = false;

        for (const auto& item : menu_sp->get_items()) {
            max_title_chars = std::max(max_title_chars, static_cast<int>(item.get_title().size()));
            if (!item.get_shortcut().empty()) {
                max_shortcut_chars = std::max(max_shortcut_chars, static_cast<int>(item.get_shortcut().size()));
            }
            if (!item.get_icon().empty()) any_icon = true;
            if (item.get_type() == MenuItemType::Checkable || item.get_type() == MenuItemType::Radio || item.is_checked()) {
                any_check = true;
            }
        }

        int approx_title_w = max_title_chars * (font_size * 0.7);
        int approx_shortcut_w = max_shortcut_chars > 0 ? (max_shortcut_chars * (font_size * 0.65) + 16) : 0;
        int icon_col_w = any_icon ? 28 : 0;
        int check_col_w = any_check ? 24 : 0;
        int pad_w = 32;

        int calculated_w = icon_col_w + approx_title_w + approx_shortcut_w + check_col_w + pad_w;
        max_w = std::max(max_w, calculated_w);

        // Height calculation for visible rows
        int total_rows = static_cast<int>(menu_sp->get_items().size());
        int visible_rows = std::min(total_rows, m_max_visible_items);
        int approx_h = (visible_rows * m_item_height) + 8;

        return Size(max_w, approx_h);
    }

    void draw(cairo_t* cr, const Rect& bounds) override {
        auto menu_sp = m_menu.lock();
        if (!cr || !menu_sp || bounds.width <= 0 || bounds.height <= 0) return;

        auto config = Config::get();
        cairo_save(cr);

        std::string font_family = !config->metrics.font_family.empty() ? config->metrics.font_family : "Sans";
        int font_size = config->metrics.font_size > 0 ? config->metrics.font_size : 11;

        const auto& items = menu_sp->get_items();
        int total_items = static_cast<int>(items.size());
        int curr_y = bounds.y + 4;

        bool any_icon = false;
        for (const auto& it : items) {
            if (!it.get_icon().empty()) { any_icon = true; break; }
        }

        for (int i = 0; i < total_items; ++i) {
            const auto& item = items[i];
            int row_h = get_row_height(item);
            int item_draw_y = curr_y - m_scroll_offset;
            curr_y += row_h;

            // Cull off-screen rows
            if (item_draw_y + row_h < bounds.y || item_draw_y > bounds.y + bounds.height) {
                continue;
            }

            Rect item_rect(bounds.x + 4, item_draw_y, bounds.width - 8, row_h);

            if (item.get_type() == MenuItemType::Separator) {
                draw_separator(cr, item_rect, config);
            } else if (item.get_type() == MenuItemType::SectionHeader) {
                draw_section_header(cr, item_rect, item, font_family, font_size, config);
            } else {
                draw_menu_action(cr, item_rect, item, i, font_family, font_size, config, menu_sp, any_icon);
            }
        }

        cairo_restore(cr);
    }

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override {
        auto menu_sp = m_menu.lock();
        if (!menu_sp) return false;
        int idx = hit_test_item(ly, bounds);
        if (idx >= 0 && idx < static_cast<int>(menu_sp->get_items().size())) {
            if (!menu_sp->get_items()[idx].is_actionable()) {
                idx = -1;
            }
        }

        if (idx != m_hovered_index) {
            m_hovered_index = idx;
            request_redraw();
            return true;
        }
        return false;
    }

    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override {
        if (button != MouseButton::Left || !pressed) return false;
        auto menu_sp = m_menu.lock();
        if (!menu_sp) return false;

        int idx = hit_test_item(ly, bounds);
        if (idx >= 0 && idx < static_cast<int>(menu_sp->get_items().size())) {
            if (menu_sp->get_items()[idx].is_actionable()) {
                menu_sp->handle_item_triggered(idx);
                return true;
            }
        }
        return false;
    }

    bool on_scroll(double delta) override {
        int max_scroll = std::max(0, get_total_content_height() - (m_max_visible_items * m_item_height));
        if (max_scroll <= 0) return false;

        int scroll_step = m_item_height;
        int new_offset = m_scroll_offset + static_cast<int>(delta * scroll_step);
        new_offset = std::clamp(new_offset, 0, max_scroll);

        if (new_offset != m_scroll_offset) {
            m_scroll_offset = new_offset;
            request_redraw();
            return true;
        }
        return false;
    }

    bool on_key(const KeyPressEvent& event) override {
        if (!event.pressed) return false;
        auto menu_sp = m_menu.lock();
        if (!menu_sp || menu_sp->get_items().empty()) return false;

        if (event.keysym == XKB_KEY_Up) {
            int next = find_next_actionable(m_hovered_index, -1);
            if (next >= 0) {
                m_hovered_index = next;
                ensure_visible(m_hovered_index);
                request_redraw();
                return true;
            }
        } else if (event.keysym == XKB_KEY_Down) {
            int next = find_next_actionable(m_hovered_index, 1);
            if (next >= 0) {
                m_hovered_index = next;
                ensure_visible(m_hovered_index);
                request_redraw();
                return true;
            }
        } else if (event.keysym == XKB_KEY_Home) {
            int next = find_next_actionable(-1, 1);
            if (next >= 0) {
                m_hovered_index = next;
                ensure_visible(m_hovered_index);
                request_redraw();
                return true;
            }
        } else if (event.keysym == XKB_KEY_End) {
            int next = find_next_actionable(static_cast<int>(menu_sp->get_items().size()), -1);
            if (next >= 0) {
                m_hovered_index = next;
                ensure_visible(m_hovered_index);
                request_redraw();
                return true;
            }
        } else if (event.keysym == XKB_KEY_Return || event.keysym == XKB_KEY_KP_Enter || event.keysym == XKB_KEY_space) {
            if (m_hovered_index >= 0 && m_hovered_index < static_cast<int>(menu_sp->get_items().size())) {
                if (menu_sp->get_items()[m_hovered_index].is_actionable()) {
                    menu_sp->handle_item_triggered(m_hovered_index);
                    return true;
                }
            }
        } else if (event.keysym == XKB_KEY_Escape) {
            menu_sp->dismiss();
            return true;
        }

        return false;
    }

private:
    void draw_separator(cairo_t* cr, const Rect& rect, const std::shared_ptr<Config>& config) const {
        double line_y = rect.y + rect.height / 2.0;
        cairo_set_source_rgba(cr, config->colors.outline_variant.r,
                                  config->colors.outline_variant.g,
                                  config->colors.outline_variant.b,
                                  0.45f);
        cairo_set_line_width(cr, 1.0);
        cairo_move_to(cr, rect.x + 8, line_y);
        cairo_line_to(cr, rect.x + rect.width - 8, line_y);
        cairo_stroke(cr);
    }

    void draw_section_header(cairo_t* cr, const Rect& rect, const MenuItem& item,
                             const std::string& font_family, int font_size,
                             const std::shared_ptr<Config>& config) const {
        PangoLayoutPtr layout(pango_cairo_create_layout(cr), g_object_unref);
        pango_layout_set_text(layout.get(), item.get_title().c_str(), -1);

        int header_font_size = std::max(9, font_size - 1);
        std::string desc_str = font_family + " Bold " + std::to_string(header_font_size);
        PangoFontDescPtr desc(pango_font_description_from_string(desc_str.c_str()), pango_font_description_free);
        pango_layout_set_font_description(layout.get(), desc.get());

        int tw = 0, th = 0;
        pango_layout_get_pixel_size(layout.get(), &tw, &th);

        int text_x = rect.x + 12;
        int text_y = rect.y + (rect.height - th) / 2;

        cairo_move_to(cr, text_x, text_y);
        cairo_set_source_rgba(cr, config->colors.on_surface_variant.r,
                                  config->colors.on_surface_variant.g,
                                  config->colors.on_surface_variant.b,
                                  0.75f);
        pango_cairo_show_layout(cr, layout.get());
    }

    void draw_menu_action(cairo_t* cr, const Rect& rect, const MenuItem& item,
                          int index, const std::string& font_family, int font_size,
                          const std::shared_ptr<Config>& config,
                          const std::shared_ptr<PopupMenu>& menu,
                          bool any_icon) const {
        bool is_hovered = (index == m_hovered_index);
        bool is_selected = (index == menu->get_selected_index()) || item.is_checked();
        bool is_destructive = (item.get_type() == MenuItemType::DestructiveAction);
        bool is_enabled = item.is_enabled();

        // 1. Pill Background
        if (is_hovered && is_enabled) {
            CardView::draw_rounded_rect(cr, rect.x, rect.y, rect.width, rect.height, 6);
            if (is_destructive) {
                cairo_set_source_rgba(cr, DANGER_COLOR.r,
                                          DANGER_COLOR.g,
                                          DANGER_COLOR.b,
                                          0.18f);
            } else {
                cairo_set_source_rgba(cr, config->colors.surface_variant.r,
                                          config->colors.surface_variant.g,
                                          config->colors.surface_variant.b,
                                          0.80f);
            }
            cairo_fill(cr);
        } else if (is_selected && is_enabled && item.get_type() != MenuItemType::Checkable) {
            CardView::draw_rounded_rect(cr, rect.x, rect.y, rect.width, rect.height, 6);
            cairo_set_source_rgba(cr, config->colors.primary.r,
                                      config->colors.primary.g,
                                      config->colors.primary.b,
                                      0.16f);
            cairo_fill(cr);
        }

        // Color selection
        Color text_col = config->colors.on_surface;
        if (!is_enabled) {
            text_col = config->colors.on_surface.with_alpha(0.38f);
        } else if (is_destructive) {
            text_col = DANGER_COLOR;
        } else if (is_selected) {
            text_col = config->colors.primary;
        }

        int curr_x = rect.x + 12;

        // 2. Icon (if present or column alignment)
        if (!item.get_icon().empty()) {
            std::string res_path = ImageView::resolve_icon_path(item.get_icon());
            bool is_img_icon = !res_path.empty() || item.get_icon().starts_with('/') ||
                               item.get_icon().ends_with(".png") || item.get_icon().ends_with(".svg");

            if (is_img_icon) {
                int icon_sz = 16;
                int icon_y = rect.y + (rect.height - icon_sz) / 2;
                bool is_symbolic = (item.get_icon().find("symbolic") != std::string::npos);

                ImageView temp_img(!res_path.empty() ? res_path : item.get_icon());
                temp_img.set_target_size(icon_sz);

                if (is_symbolic) {
                    cairo_save(cr);
                    cairo_push_group(cr);
                    temp_img.draw(cr, Rect(curr_x, icon_y, icon_sz, icon_sz));
                    cairo_pattern_t* icon_pattern = cairo_pop_group(cr);

                    cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
                    cairo_mask(cr, icon_pattern);
                    cairo_pattern_destroy(icon_pattern);
                    cairo_restore(cr);
                } else {
                    temp_img.draw(cr, Rect(curr_x, icon_y, icon_sz, icon_sz));
                }
                curr_x += 24;
            } else if (item.get_icon().size() <= 8) {
                // Short emoji or single glyph symbol
                PangoLayoutPtr icon_layout(pango_cairo_create_layout(cr), g_object_unref);
                pango_layout_set_text(icon_layout.get(), item.get_icon().c_str(), -1);

                std::string icon_desc = font_family + " " + std::to_string(font_size);
                PangoFontDescPtr idesc(pango_font_description_from_string(icon_desc.c_str()), pango_font_description_free);
                pango_layout_set_font_description(icon_layout.get(), idesc.get());

                int iw = 0, ih = 0;
                pango_layout_get_pixel_size(icon_layout.get(), &iw, &ih);
                int icon_y = rect.y + (rect.height - ih) / 2;

                cairo_move_to(cr, curr_x, icon_y);
                cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
                pango_cairo_show_layout(cr, icon_layout.get());
                curr_x += 24;
            }
        } else if (any_icon && item.get_type() != MenuItemType::Checkable && item.get_type() != MenuItemType::Radio) {
            curr_x += 24;
        }

        // 3. Indicator for checkable/radio items
        if (item.get_type() == MenuItemType::Checkable) {
            int chk_sz = 14;
            int chk_y = rect.y + (rect.height - chk_sz) / 2;
            CardView::draw_rounded_rect(cr, curr_x, chk_y, chk_sz, chk_sz, 3);
            if (item.is_checked()) {
                cairo_set_source_rgba(cr, config->colors.primary.r,
                                          config->colors.primary.g,
                                          config->colors.primary.b,
                                          config->colors.primary.a);
                cairo_fill(cr);

                // Small check symbol
                cairo_set_source_rgba(cr, config->colors.on_primary.r,
                                          config->colors.on_primary.g,
                                          config->colors.on_primary.b,
                                          1.0f);
                cairo_set_line_width(cr, 1.8);
                cairo_move_to(cr, curr_x + 3, chk_y + 7);
                cairo_line_to(cr, curr_x + 6, chk_y + 10);
                cairo_line_to(cr, curr_x + 11, chk_y + 4);
                cairo_stroke(cr);
            } else {
                cairo_set_source_rgba(cr, config->colors.outline.r,
                                          config->colors.outline.g,
                                          config->colors.outline.b,
                                          0.60f);
                cairo_set_line_width(cr, 1.2);
                cairo_stroke(cr);
            }
            curr_x += chk_sz + 8;
        } else if (item.get_type() == MenuItemType::Radio) {
            int rad_sz = 14;
            double center_x = curr_x + rad_sz / 2.0;
            double center_y = rect.y + rect.height / 2.0;
            double radius = rad_sz / 2.0;

            cairo_new_sub_path(cr);
            cairo_arc(cr, center_x, center_y, radius, 0, 2 * M_PI);
            if (item.is_checked()) {
                cairo_set_source_rgba(cr, config->colors.primary.r,
                                          config->colors.primary.g,
                                          config->colors.primary.b,
                                          config->colors.primary.a);
                cairo_set_line_width(cr, 1.8);
                cairo_stroke(cr);

                cairo_new_sub_path(cr);
                cairo_arc(cr, center_x, center_y, radius * 0.5, 0, 2 * M_PI);
                cairo_set_source_rgba(cr, config->colors.primary.r,
                                          config->colors.primary.g,
                                          config->colors.primary.b,
                                          config->colors.primary.a);
                cairo_fill(cr);
            } else {
                cairo_set_source_rgba(cr, config->colors.outline.r,
                                          config->colors.outline.g,
                                          config->colors.outline.b,
                                          0.60f);
                cairo_set_line_width(cr, 1.2);
                cairo_stroke(cr);
            }
            curr_x += rad_sz + 8;
        }

        int right_x = rect.x + rect.width - 12;

        // 4. Shortcut Text (Right-aligned if present)
        if (!item.get_shortcut().empty()) {
            PangoLayoutPtr sc_layout(pango_cairo_create_layout(cr), g_object_unref);
            pango_layout_set_text(sc_layout.get(), item.get_shortcut().c_str(), -1);

            int sc_font_size = std::max(9, font_size - 1);
            std::string sc_desc_str = font_family + " " + std::to_string(sc_font_size);
            PangoFontDescPtr sc_desc(pango_font_description_from_string(sc_desc_str.c_str()), pango_font_description_free);
            pango_layout_set_font_description(sc_layout.get(), sc_desc.get());

            int sw = 0, sh = 0;
            pango_layout_get_pixel_size(sc_layout.get(), &sw, &sh);
            right_x -= sw;
            int sc_y = rect.y + (rect.height - sh) / 2;

            Color sc_col = is_enabled ? config->colors.on_surface_variant.with_alpha(0.70f)
                                      : config->colors.on_surface.with_alpha(0.30f);
            cairo_move_to(cr, right_x, sc_y);
            cairo_set_source_rgba(cr, sc_col.r, sc_col.g, sc_col.b, sc_col.a);
            pango_cairo_show_layout(cr, sc_layout.get());
            right_x -= 12;
        }

        // 5. Title Text (Ellipsized between curr_x and right_x)
        int avail_title_w = std::max(20, right_x - curr_x);
        PangoLayoutPtr title_layout(pango_cairo_create_layout(cr), g_object_unref);
        pango_layout_set_text(title_layout.get(), item.get_title().c_str(), -1);

        std::string title_desc = font_family + " " + std::to_string(font_size);
        if (is_selected && item.get_type() != MenuItemType::Checkable) title_desc += " Bold";
        PangoFontDescPtr tdesc(pango_font_description_from_string(title_desc.c_str()), pango_font_description_free);
        pango_layout_set_font_description(title_layout.get(), tdesc.get());

        pango_layout_set_width(title_layout.get(), avail_title_w * PANGO_SCALE);
        pango_layout_set_ellipsize(title_layout.get(), PANGO_ELLIPSIZE_END);

        int tw = 0, th = 0;
        pango_layout_get_pixel_size(title_layout.get(), &tw, &th);
        int title_y = rect.y + (rect.height - th) / 2;

        cairo_move_to(cr, curr_x, title_y);
        cairo_set_source_rgba(cr, text_col.r, text_col.g, text_col.b, text_col.a);
        pango_cairo_show_layout(cr, title_layout.get());
    }

    int hit_test_item(int ly, const Rect& bounds) const {
        auto menu_sp = m_menu.lock();
        if (!menu_sp) return -1;
        int rel_y = ly - (bounds.y + 4) + m_scroll_offset;
        if (rel_y < 0) return -1;

        int curr_y = 0;
        const auto& items = menu_sp->get_items();
        for (size_t i = 0; i < items.size(); ++i) {
            int h = get_row_height(items[i]);
            if (rel_y >= curr_y && rel_y < curr_y + h) {
                return static_cast<int>(i);
            }
            curr_y += h;
        }
        return -1;
    }

    int find_next_actionable(int from_idx, int direction) const {
        auto menu_sp = m_menu.lock();
        if (!menu_sp || menu_sp->get_items().empty()) return -1;
        int count = static_cast<int>(menu_sp->get_items().size());
        int curr = from_idx + direction;

        for (int step = 0; step < count; ++step) {
            if (curr < 0) curr = count - 1;
            else if (curr >= count) curr = 0;

            if (menu_sp->get_items()[curr].is_actionable()) {
                return curr;
            }
            curr += direction;
        }
        return -1;
    }

    void ensure_visible(int index) {
        auto menu_sp = m_menu.lock();
        if (!menu_sp || index < 0 || index >= static_cast<int>(menu_sp->get_items().size())) return;

        int item_top = 0;
        const auto& items = menu_sp->get_items();
        for (int i = 0; i < index; ++i) {
            item_top += get_row_height(items[i]);
        }
        int item_bottom = item_top + get_row_height(items[index]);
        int visible_h = m_max_visible_items * m_item_height;

        if (item_top < m_scroll_offset) {
            m_scroll_offset = item_top;
        } else if (item_bottom > m_scroll_offset + visible_h) {
            m_scroll_offset = item_bottom - visible_h;
        }
    }

    std::weak_ptr<PopupMenu> m_menu;
    int m_hovered_index = -1;
    int m_scroll_offset = 0;
    int m_item_height = 36;
    int m_max_visible_items = 8;
};

} // anonymous namespace

// =============================================================================
// MenuItem Implementation
// =============================================================================

MenuItem::MenuItem(std::string title, std::function<void()> on_click)
    : m_type(MenuItemType::Action), m_title(std::move(title)), m_on_click(std::move(on_click)) {}

MenuItem::MenuItem(std::string title, std::string icon, std::function<void()> on_click)
    : m_type(MenuItemType::Action), m_title(std::move(title)), m_icon(std::move(icon)), m_on_click(std::move(on_click)) {}

MenuItem::MenuItem(std::string title, std::string icon, std::string shortcut, std::function<void()> on_click)
    : m_type(MenuItemType::Action), m_title(std::move(title)), m_icon(std::move(icon)), m_shortcut(std::move(shortcut)), m_on_click(std::move(on_click)) {}

void MenuItem::trigger() {
    if (!m_enabled) return;

    if (m_type == MenuItemType::Checkable) {
        m_checked = !m_checked;
        if (m_on_toggled) m_on_toggled(m_checked);
    } else if (m_type == MenuItemType::Radio) {
        m_checked = true;
        if (m_on_toggled) m_on_toggled(m_checked);
    }

    if (m_on_click) {
        m_on_click();
    }
}

// =============================================================================
// PopupMenu Implementation
// =============================================================================

PopupMenu::PopupMenu() = default;

PopupMenu::~PopupMenu() {
    dismiss();
}

void PopupMenu::add_item(MenuItem item) {
    m_items.push_back(std::move(item));
}

void PopupMenu::add_item(std::string title, std::function<void()> on_click) {
    m_items.emplace_back(std::move(title), std::move(on_click));
}

void PopupMenu::add_item(std::string title, std::string icon, std::function<void()> on_click) {
    m_items.emplace_back(std::move(title), std::move(icon), std::move(on_click));
}

void PopupMenu::add_item(std::string title, std::string icon, std::string shortcut, std::function<void()> on_click) {
    m_items.emplace_back(std::move(title), std::move(icon), std::move(shortcut), std::move(on_click));
}

void PopupMenu::add_checkable_item(std::string title, bool checked, std::function<void(bool)> on_toggled) {
    MenuItem item(std::move(title), nullptr);
    item.set_type(MenuItemType::Checkable);
    item.set_checked(checked);
    item.set_on_toggled(std::move(on_toggled));
    m_items.push_back(std::move(item));
}

void PopupMenu::add_radio_item(std::string title, int group_id, bool checked, std::function<void(bool)> on_toggled) {
    MenuItem item(std::move(title), nullptr);
    item.set_type(MenuItemType::Radio);
    item.set_group_id(group_id);
    item.set_checked(checked);
    item.set_on_toggled(std::move(on_toggled));
    m_items.push_back(std::move(item));
}

void PopupMenu::add_destructive_item(std::string title, std::string icon, std::function<void()> on_click) {
    MenuItem item(std::move(title), std::move(icon), std::move(on_click));
    item.set_type(MenuItemType::DestructiveAction);
    m_items.push_back(std::move(item));
}

void PopupMenu::add_separator() {
    MenuItem item;
    item.set_type(MenuItemType::Separator);
    item.set_enabled(false);
    m_items.push_back(std::move(item));
}

void PopupMenu::add_section_header(std::string title) {
    MenuItem item;
    item.set_title(std::move(title));
    item.set_type(MenuItemType::SectionHeader);
    item.set_enabled(false);
    m_items.push_back(std::move(item));
}

void PopupMenu::clear_items() {
    m_items.clear();
    m_selected_index = -1;
    dismiss();
}

void PopupMenu::set_selected_index(int index) {
    if (index >= -1 && index < static_cast<int>(m_items.size())) {
        m_selected_index = index;
        if (m_menu_view) {
            m_menu_view->request_redraw();
        }
    }
}

void PopupMenu::handle_item_triggered(int index) {
    auto self = shared_from_this();
    if (index < 0 || index >= static_cast<int>(m_items.size())) return;

    auto& item = m_items[index];
    if (!item.is_actionable()) return;

    // Radio button uncheck others in group
    if (item.get_type() == MenuItemType::Radio) {
        int gid = item.get_group_id();
        for (size_t i = 0; i < m_items.size(); ++i) {
            if (static_cast<int>(i) != index && m_items[i].get_type() == MenuItemType::Radio &&
                m_items[i].get_group_id() == gid) {
                m_items[i].set_checked(false);
            }
        }
    }

    set_selected_index(index);
    item.trigger();

    if (m_on_item_selected) {
        m_on_item_selected(index, item);
    }

    if (m_dismiss_on_select && item.get_type() != MenuItemType::Checkable) {
        dismiss();
    } else if (m_menu_view) {
        m_menu_view->request_redraw();
    }
}

void PopupMenu::ensure_popup_window(Window* win) {
    if (m_popup_window) return;

    m_menu_view = std::make_shared<PopupMenuView>(weak_from_this(), m_item_height, m_max_visible_items);

    Size measured = m_menu_view->measure_size();
    int popup_w = std::max(m_min_width, measured.width);
    int popup_h = measured.height;

    m_popup_window = PopupWindowBuilder::create()
        ->content(m_menu_view)
        ->size(popup_w, popup_h)
        ->elevation(5)
        ->autoFlip(true)
        ->autoClamp(true)
        ->onDismiss([weak_self = weak_from_this()]() {
            if (auto self = weak_self.lock()) {
                self->m_popup_window = nullptr;
                self->m_menu_view = nullptr;
                if (self->m_on_dismiss) {
                    auto cb = std::move(self->m_on_dismiss);
                    self->m_on_dismiss = nullptr;
                    cb();
                }
            }
        })
        ->build();
}

void PopupMenu::show_as_dropdown(const std::shared_ptr<View>& anchor,
                                 PopupGravity gravity,
                                 int x_offset, int y_offset) {
    if (!anchor) return;
    show_as_dropdown(anchor.get(), gravity, x_offset, y_offset);
}

void PopupMenu::show_as_dropdown(View* anchor,
                                 PopupGravity gravity,
                                 int x_offset, int y_offset) {
    if (!anchor || m_items.empty()) return;
    Window* win = anchor->get_window();
    if (!win) return;

    // If anchor is wider than min_width, match anchor width
    Rect anchor_rect = anchor->get_bounds();
    if (anchor_rect.width > m_min_width) {
        m_min_width = anchor_rect.width;
    }

    ensure_popup_window(win);
    m_popup_window->show_as_dropdown(anchor, gravity, x_offset, y_offset);
}

void PopupMenu::show_at_location(Window* window, int x, int y) {
    if (!window || m_items.empty()) return;
    ensure_popup_window(window);
    m_popup_window->show_at_location(window, x, y);
}

void PopupMenu::dismiss() {
    if (m_popup_window) {
        auto win = m_popup_window;
        m_popup_window = nullptr;
        m_menu_view = nullptr;
        win->dismiss();
    }
}

bool PopupMenu::is_showing() const {
    return m_popup_window && m_popup_window->is_showing();
}

// =============================================================================
// PopupMenuBuilder Implementation
// =============================================================================

PopupMenuBuilder::PopupMenuBuilder()
    : m_menu(std::make_shared<PopupMenu>()) {}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::create() {
    return std::make_shared<PopupMenuBuilder>();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::item(std::string title, std::function<void()> on_click) {
    m_menu->add_item(std::move(title), std::move(on_click));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::item(std::string title, std::string icon, std::function<void()> on_click) {
    m_menu->add_item(std::move(title), std::move(icon), std::move(on_click));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::item(std::string title, std::string icon, std::string shortcut, std::function<void()> on_click) {
    m_menu->add_item(std::move(title), std::move(icon), std::move(shortcut), std::move(on_click));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::checkable(std::string title, bool checked, std::function<void(bool)> on_toggled) {
    m_menu->add_checkable_item(std::move(title), checked, std::move(on_toggled));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::radio(std::string title, int group_id, bool checked, std::function<void(bool)> on_toggled) {
    m_menu->add_radio_item(std::move(title), group_id, checked, std::move(on_toggled));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::destructive(std::string title, std::string icon, std::function<void()> on_click) {
    m_menu->add_destructive_item(std::move(title), std::move(icon), std::move(on_click));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::separator() {
    m_menu->add_separator();
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::section(std::string title) {
    m_menu->add_section_header(std::move(title));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::minWidth(int width) {
    m_menu->set_min_width(width);
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::maxVisibleItems(int count) {
    m_menu->set_max_visible_items(count);
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::itemHeight(int height) {
    m_menu->set_item_height(height);
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::dismissOnSelect(bool dismiss) {
    m_menu->set_dismiss_on_select(dismiss);
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::onDismiss(std::function<void()> listener) {
    m_menu->set_on_dismiss(std::move(listener));
    return shared_from_this();
}

std::shared_ptr<PopupMenuBuilder> PopupMenuBuilder::onItemSelected(std::function<void(int, const MenuItem&)> listener) {
    m_menu->set_on_item_selected(std::move(listener));
    return shared_from_this();
}

std::shared_ptr<PopupMenu> PopupMenuBuilder::build() {
    return m_menu;
}

} // namespace miqu
