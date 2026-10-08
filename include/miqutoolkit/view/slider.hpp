#pragma once

#include "miqutoolkit/view/view.hpp"
#include <functional>
#include <algorithm>

namespace miqu {

class Slider : public View {
public:
    Slider() = default;

    void set_value(float value) {
        float clamped = std::clamp(value, 0.0f, 1.0f);
        if (std::abs(m_value - clamped) > 0.0001f) {
            m_value = clamped;
            request_redraw();
        }
    }
    float get_value() const { return m_value; }

    // Convenience alias for media players
    void set_progress(float p) { set_value(p); }
    float get_progress() const { return get_value(); }

    void set_track_color(const Color& col) { m_track_color = col; m_custom_track = true; request_redraw(); }
    void set_progress_color(const Color& col) { m_progress_color = col; m_custom_progress = true; request_redraw(); }
    void set_thumb_color(const Color& col) { m_thumb_color = col; m_custom_thumb = true; request_redraw(); }

    void set_track_height(int height) { m_track_height = std::max(1, height); request_redraw(); }
    int get_track_height() const { return m_track_height; }

    void set_thumb_radius(int radius) { m_thumb_radius = std::max(0, radius); request_redraw(); }
    int get_thumb_radius() const { return m_thumb_radius; }

    void set_on_value_changed_listener(std::function<void(float value, bool from_user)> listener) {
        m_on_change = std::move(listener);
    }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;

private:
    float m_value = 0.0f;
    int m_track_height = 4;
    int m_thumb_radius = 7;

    bool m_custom_track = false;
    Color m_track_color;

    bool m_custom_progress = false;
    Color m_progress_color;

    bool m_custom_thumb = false;
    Color m_thumb_color;

    bool m_dragging = false;
    bool m_hovered = false;

    std::function<void(float, bool)> m_on_change;
};

class SliderBuilder : public std::enable_shared_from_this<SliderBuilder> {
public:
    SliderBuilder() : m_view(std::make_shared<Slider>()) {}

    static std::shared_ptr<SliderBuilder> create() {
        return std::make_shared<SliderBuilder>();
    }

    std::shared_ptr<SliderBuilder> value(float v) {
        m_view->set_value(v);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> progress(float p) {
        m_view->set_progress(p);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> trackColor(const Color& col) {
        m_view->set_track_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> progressColor(const Color& col) {
        m_view->set_progress_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> thumbColor(const Color& col) {
        m_view->set_thumb_color(col);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> trackHeight(int h) {
        m_view->set_track_height(h);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> thumbRadius(int r) {
        m_view->set_thumb_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> onValueChanged(std::function<void(float, bool)> listener) {
        m_view->set_on_value_changed_listener(std::move(listener));
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> onSeek(std::function<void(float, bool)> listener) {
        m_view->set_on_value_changed_listener(std::move(listener));
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<SliderBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<Slider> build() {
        return m_view;
    }

private:
    std::shared_ptr<Slider> m_view;
};

} // namespace miqu
