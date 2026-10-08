#pragma once

#include "miqutoolkit/view/view_group.hpp"
#include <vector>
#include <memory>
#include <string>
#include <functional>

namespace miqu {

struct NavigationPage {
    std::shared_ptr<View> view;
    std::string title;
    std::string subtitle;
    std::string tag;
};

class NavigationView : public ViewGroup {
public:
    NavigationView() = default;
    ~NavigationView() override = default;

    // --- Navigation Stack Operations ---
    void push(std::shared_ptr<View> view, const std::string& title = "", const std::string& tag = "");
    void push(std::shared_ptr<View> view, const std::string& title, const std::string& subtitle, const std::string& tag);

    bool pop();
    bool pop_to_root();
    bool pop_to_tag(const std::string& tag);
    void replace_top(std::shared_ptr<View> view, const std::string& title = "", const std::string& tag = "");

    // --- State Queries ---
    size_t get_depth() const { return m_stack.size(); }
    bool can_go_back() const { return m_stack.size() > 1; }
    bool is_empty() const { return m_stack.empty(); }

    std::shared_ptr<View> get_current_view() const;
    const std::string& get_current_title() const;
    const std::string& get_current_subtitle() const;
    const std::string& get_current_tag() const;
    const NavigationPage* get_current_page() const;

    std::shared_ptr<View> get_root_view() const;
    const std::vector<NavigationPage>& get_stack() const { return m_stack; }

    // --- Desktop Shortcut Automation ---
    void set_auto_back(bool enable = true) { m_auto_back = enable; }
    bool get_auto_back() const { return m_auto_back; }

    void set_back_on_escape(bool enable = true) { m_back_on_escape = enable; }
    bool get_back_on_escape() const { return m_back_on_escape; }

    // --- Callbacks ---
    void set_on_navigation_listener(std::function<void(const NavigationPage& current_page, bool can_go_back)> cb) {
        m_on_navigation = std::move(cb);
    }

    // --- View Overrides ---
    Size measure_size() const override;
    Size measure_size(int avail_width) const override;
    void draw(cairo_t* cr, const Rect& bounds) override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_scroll(double delta) override;
    bool on_key(const KeyPressEvent& event) override;
    bool on_touch(const TouchEvent& event, const Rect& bounds) override;

    void set_window(Window* win) override;

private:
    void notify_navigation_changed();

    std::vector<NavigationPage> m_stack;
    bool m_auto_back = true;
    bool m_back_on_escape = true;
    std::function<void(const NavigationPage&, bool)> m_on_navigation;
    static const std::string s_empty_str;
};

class NavigationViewBuilder : public std::enable_shared_from_this<NavigationViewBuilder> {
public:
    NavigationViewBuilder() : m_view(std::make_shared<NavigationView>()) {}

    static std::shared_ptr<NavigationViewBuilder> create() {
        return std::make_shared<NavigationViewBuilder>();
    }

    std::shared_ptr<NavigationViewBuilder> root(std::shared_ptr<View> view, const std::string& title = "", const std::string& tag = "") {
        m_view->push(std::move(view), title, tag);
        return shared_from_this();
    }

    std::shared_ptr<NavigationViewBuilder> autoBack(bool enable = true) {
        m_view->set_auto_back(enable);
        return shared_from_this();
    }

    std::shared_ptr<NavigationViewBuilder> backOnEscape(bool enable = true) {
        m_view->set_back_on_escape(enable);
        return shared_from_this();
    }

    std::shared_ptr<NavigationViewBuilder> onNavigation(std::function<void(const NavigationPage&, bool)> cb) {
        m_view->set_on_navigation_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<NavigationView> build() {
        return m_view;
    }

private:
    std::shared_ptr<NavigationView> m_view;
};

} // namespace miqu
