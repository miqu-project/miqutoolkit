#pragma once

#include "miqutoolkit/view/view_group.hpp"
#include <vector>
#include <memory>
#include <functional>

namespace miqu {

class ViewPager : public ViewGroup {
public:
    ViewPager() = default;

    void add_page(std::shared_ptr<View> page);
    void set_current_page(int index);
    int get_current_page() const { return m_current_page; }
    size_t get_page_count() const { return m_pages.size(); }
    std::shared_ptr<View> get_page(int index) const;

    void set_on_page_changed_listener(std::function<void(int old_idx, int new_idx)> cb) {
        m_on_page_changed = std::move(cb);
    }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_scroll(double delta) override;
    bool on_key(const KeyPressEvent& event) override;
    bool on_touch(const TouchEvent& event, const Rect& bounds) override;

private:
    std::vector<std::shared_ptr<View>> m_pages;
    int m_current_page = 0;
    std::function<void(int, int)> m_on_page_changed;
};

class ViewPagerBuilder : public std::enable_shared_from_this<ViewPagerBuilder> {
public:
    ViewPagerBuilder();

    static std::shared_ptr<ViewPagerBuilder> create() {
        return std::make_shared<ViewPagerBuilder>();
    }

    std::shared_ptr<ViewPagerBuilder> addPage(std::shared_ptr<View> page) {
        m_view->add_page(std::move(page));
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> currentPage(int page) {
        m_view->set_current_page(page);
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> onPageChanged(std::function<void(int, int)> cb) {
        m_view->set_on_page_changed_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ViewPagerBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ViewPager> build() {
        return m_view;
    }

private:
    std::shared_ptr<ViewPager> m_view;
};

} // namespace miqu
