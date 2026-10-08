#pragma once

#include "miqutoolkit/view/view.hpp"
#include <algorithm>

namespace miqu {

enum class ProgressBarStyle {
    Linear,
    Circular,
};

class ProgressBar : public View {
public:
    ProgressBar() = default;

    void set_progress(float progress) {
        float clamped = std::clamp(progress, 0.0f, 1.0f);
        if (std::abs(m_progress - clamped) > 0.0001f) {
            m_progress = clamped;
            request_redraw();
        }
    }
    float get_progress() const { return m_progress; }

    void set_indeterminate(bool indeterminate);
    bool is_indeterminate() const { return m_indeterminate; }

    void set_style(ProgressBarStyle style) { m_style = style; request_redraw(); }
    ProgressBarStyle get_style() const { return m_style; }

    void set_circular(bool circular) {
        m_style = circular ? ProgressBarStyle::Circular : ProgressBarStyle::Linear;
        request_redraw();
    }
    bool is_circular() const { return m_style == ProgressBarStyle::Circular; }

    void set_track_height(int h) { m_track_height = std::max(1, h); request_redraw(); }
    int get_track_height() const { return m_track_height; }

    void set_stroke_width(int w) { m_track_height = std::max(1, w); request_redraw(); }
    int get_stroke_width() const { return m_track_height; }

    void set_corner_radius(int r) { m_corner_radius = r; request_redraw(); }
    int get_corner_radius() const { return m_corner_radius; }

    void set_track_color(const Color& col) { m_track_color = col; m_has_custom_track = true; request_redraw(); }
    void set_progress_color(const Color& col) { m_progress_color = col; m_has_custom_progress = true; request_redraw(); }

    const Color& get_track_color() const { return m_track_color; }
    const Color& get_progress_color() const { return m_progress_color; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override;

private:
    float m_progress = 0.0f;
    bool m_indeterminate = false;
    ProgressBarStyle m_style = ProgressBarStyle::Linear;
    int m_track_height = 8;
    int m_corner_radius = -1; // -1 = fully rounded pill (track_height / 2)

    bool m_has_custom_track = false;
    Color m_track_color;

    bool m_has_custom_progress = false;
    Color m_progress_color;
};

class ProgressBarBuilder : public std::enable_shared_from_this<ProgressBarBuilder> {
public:
    ProgressBarBuilder() : m_bar(std::make_shared<ProgressBar>()) {}

    static std::shared_ptr<ProgressBarBuilder> create() {
        return std::make_shared<ProgressBarBuilder>();
    }

    std::shared_ptr<ProgressBarBuilder> progress(float p) {
        m_bar->set_progress(p);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> indeterminate(bool ind = true) {
        m_bar->set_indeterminate(ind);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> style(ProgressBarStyle s) {
        m_bar->set_style(s);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> circular(bool circ = true) {
        m_bar->set_circular(circ);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> size(int w, int h) {
        m_bar->set_layout_params(LayoutParams(w, h));
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> trackHeight(int h) {
        m_bar->set_track_height(h);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> strokeWidth(int w) {
        m_bar->set_stroke_width(w);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> cornerRadius(int r) {
        m_bar->set_corner_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> trackColor(const Color& c) {
        m_bar->set_track_color(c);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> progressColor(const Color& c) {
        m_bar->set_progress_color(c);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> padding(const Padding& p) {
        m_bar->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> padding(int uniform) {
        m_bar->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> padding(int h, int v) {
        m_bar->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> padding(int l, int t, int r, int b) {
        m_bar->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> margin(const Margin& m) {
        m_bar->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> margin(int uniform) {
        m_bar->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> margin(int h, int v) {
        m_bar->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBarBuilder> margin(int l, int t, int r, int b) {
        m_bar->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ProgressBar> build() {
        return m_bar;
    }

private:
    std::shared_ptr<ProgressBar> m_bar;
};

} // namespace miqu
