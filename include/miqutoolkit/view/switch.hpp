#pragma once

#include "miqutoolkit/view/view.hpp"
#include <functional>

namespace miqu {

class Switch : public View {
public:
    Switch();
    ~Switch() override = default;

    void set_checked(bool checked);
    bool is_checked() const { return m_checked; }

    void toggle() { set_checked(!m_checked); }

    void set_on_checked_changed_listener(std::function<void(bool is_checked)> cb) {
        m_on_checked_changed = std::move(cb);
    }

    void set_track_active_color(const Color& col) { m_track_active_color = col; m_has_custom_track_active = true; }
    void set_track_inactive_color(const Color& col) { m_track_inactive_color = col; m_has_custom_track_inactive = true; }
    void set_thumb_active_color(const Color& col) { m_thumb_active_color = col; m_has_custom_thumb_active = true; }
    void set_thumb_inactive_color(const Color& col) { m_thumb_inactive_color = col; m_has_custom_thumb_inactive = true; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    bool m_checked = false;
    bool m_hovered = false;
    bool m_pressed = false;

    bool m_has_custom_track_active = false;
    Color m_track_active_color;

    bool m_has_custom_track_inactive = false;
    Color m_track_inactive_color;

    bool m_has_custom_thumb_active = false;
    Color m_thumb_active_color;

    bool m_has_custom_thumb_inactive = false;
    Color m_thumb_inactive_color;

    std::function<void(bool)> m_on_checked_changed;
};

class SwitchBuilder : public std::enable_shared_from_this<SwitchBuilder> {
public:
    SwitchBuilder();

    static std::shared_ptr<SwitchBuilder> create() {
        return std::make_shared<SwitchBuilder>();
    }

    std::shared_ptr<SwitchBuilder> checked(bool c) {
        m_view->set_checked(c);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> onCheckedChanged(std::function<void(bool)> cb) {
        m_view->set_on_checked_changed_listener(std::move(cb));
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> trackActiveColor(const Color& c) {
        m_view->set_track_active_color(c);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> trackInactiveColor(const Color& c) {
        m_view->set_track_inactive_color(c);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> thumbActiveColor(const Color& c) {
        m_view->set_thumb_active_color(c);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> thumbInactiveColor(const Color& c) {
        m_view->set_thumb_inactive_color(c);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SwitchBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<Switch> build() {
        return m_view;
    }

private:
    std::shared_ptr<Switch> m_view;
};

} // namespace miqu
