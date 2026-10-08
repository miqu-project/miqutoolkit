#pragma once

#include "miqutoolkit/view/view.hpp"
#include <string>

namespace miqu {

enum class FitMode {
    Contain, // Scales to fit inside bounds, preserving aspect ratio (default)
    Cover,   // Scales to fill entire bounds, preserving aspect ratio, cropped
    Fill,    // Stretches to fill bounds width & height (ignores aspect ratio)
    Center,  // Keeps target size, centered within bounds
    Tile,    // Repeats pattern across bounds
};

enum class ImageQuality {
    FullOriginal, // Default: full native original quality, direct decode
    ThumbnailFast // Explicitly requested: uses FreeDesktop persistent disk thumbnail cache (~/.cache/thumbnails)
};

class ImageView : public View {
public:
    ImageView() = default;
    explicit ImageView(std::string source) : m_source(std::move(source)) {}
    ~ImageView() override;

    ImageView(const ImageView&) = delete;
    ImageView& operator=(const ImageView&) = delete;
    ImageView(ImageView&&) = delete;
    ImageView& operator=(ImageView&&) = delete;

    void set_image_resource(std::string source) {
        if (m_source != source) {
            m_source = std::move(source);
            invalidate_surface_cache();
            request_redraw();
        }
    }
    const std::string& get_image_resource() const { return m_source; }

    void set_target_size(int size) {
        if (m_target_size != size) {
            m_target_size = size;
            invalidate_surface_cache();
            request_redraw();
        }
    }
    int get_target_size() const { return m_target_size; }

    void set_fit_mode(FitMode mode) {
        if (m_fit_mode != mode) {
            m_fit_mode = mode;
            invalidate_surface_cache();
            request_redraw();
        }
    }
    FitMode get_fit_mode() const { return m_fit_mode; }

    void set_quality_mode(ImageQuality quality) {
        if (m_quality != quality) {
            m_quality = quality;
            invalidate_surface_cache();
            request_redraw();
        }
    }
    ImageQuality get_quality_mode() const { return m_quality; }

    void set_blur_radius(int radius) {
        int clamped = std::max(0, radius);
        if (m_blur_radius != clamped) {
            m_blur_radius = clamped;
            invalidate_surface_cache();
            request_redraw();
        }
    }
    int get_blur_radius() const { return m_blur_radius; }

    void set_dim(float dim) {
        float clamped = std::clamp(dim, 0.0f, 1.0f);
        if (std::abs(m_dim_alpha - clamped) > 0.001f) {
            m_dim_alpha = clamped;
            invalidate_surface_cache();
            request_redraw();
        }
    }
    float get_dim() const { return m_dim_alpha; }

    void invalidate_surface_cache();

    void set_corner_radius(int radius) { m_corner_radius = radius; request_redraw(); }
    int get_corner_radius() const { return m_corner_radius; }

    void set_circle(bool circle) { m_circle = circle; request_redraw(); }
    bool is_circle() const { return m_circle; }

    void set_border(int width, const Color& color) {
        m_border_width = std::max(0, width);
        m_border_color = color;
        request_redraw();
    }
    int get_border_width() const { return m_border_width; }
    const Color& get_border_color() const { return m_border_color; }

    void set_rotation_angle(double degrees) { m_rotation_degrees = degrees; request_redraw(); }
    double get_rotation_angle() const { return m_rotation_degrees; }

    void set_alpha(float opacity) { m_opacity = opacity; request_redraw(); }
    float get_alpha() const { return m_opacity; }

    void set_background_color(const Color& color) { m_bg_color = color; request_redraw(); }
    const Color& get_background_color() const { return m_bg_color; }

    void draw(cairo_t* cr, const Rect& bounds) override;
    Size measure_size() const override { return Size(m_target_size, m_target_size); }

    static std::string resolve_icon_path(const std::string& icon_name);
    static void preload(const std::string& source, ImageQuality quality = ImageQuality::ThumbnailFast);
    static void preload_surface(const std::string& source, int target_size);
    static void clear_cache();

private:
    std::string m_source;
    int m_target_size = 48;
    FitMode m_fit_mode = FitMode::Contain;
    ImageQuality m_quality = ImageQuality::FullOriginal;
    int m_corner_radius = 0;
    bool m_circle = false;
    int m_border_width = 0;
    Color m_border_color = Color::transparent();
    double m_rotation_degrees = 0.0;
    float m_opacity = 1.0f;
    Color m_bg_color = Color::transparent();
    int m_blur_radius = 0;
    float m_dim_alpha = 0.0f;

    cairo_surface_t* m_cached_surface = nullptr;
    int m_cached_w = 0;
    int m_cached_h = 0;
    std::string m_cached_source;
    FitMode m_cached_fit = FitMode::Contain;
    ImageQuality m_cached_quality = ImageQuality::FullOriginal;
    int m_cached_target_size = 0;
    int m_cached_blur_radius = 0;
    float m_cached_dim_alpha = 0.0f;
};

class ImageViewBuilder : public std::enable_shared_from_this<ImageViewBuilder> {
public:
    ImageViewBuilder() : m_view(std::make_shared<ImageView>()) {}

    static std::shared_ptr<ImageViewBuilder> create() {
        return std::make_shared<ImageViewBuilder>();
    }

    std::shared_ptr<ImageViewBuilder> source(std::string src) {
        m_view->set_image_resource(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> backgroundColor(const Color& color) {
        m_view->set_background_color(color);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> qualityMode(ImageQuality quality) {
        m_view->set_quality_mode(quality);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> blurRadius(int radius) {
        m_view->set_blur_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> dim(float dim) {
        m_view->set_dim(dim);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> imageResource(std::string src) {
        m_view->set_image_resource(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> targetSize(int sz) {
        m_view->set_target_size(sz);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> fitMode(FitMode mode) {
        m_view->set_fit_mode(mode);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> cornerRadius(int radius) {
        m_view->set_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> circle(bool c = true) {
        m_view->set_circle(c);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> border(int width, const Color& color) {
        m_view->set_border(width, color);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> rotation(double degrees) {
        m_view->set_rotation_angle(degrees);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> alpha(float op) {
        m_view->set_alpha(op);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(const Padding& p) {
        m_view->set_padding(p);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(int uniform) {
        m_view->set_padding(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(int h, int v) {
        m_view->set_padding(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> padding(int l, int t, int r, int b) {
        m_view->set_padding(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(const Margin& m) {
        m_view->set_margin(m);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(int uniform) {
        m_view->set_margin(uniform);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(int h, int v) {
        m_view->set_margin(h, v);
        return shared_from_this();
    }

    std::shared_ptr<ImageViewBuilder> margin(int l, int t, int r, int b) {
        m_view->set_margin(l, t, r, b);
        return shared_from_this();
    }

    std::shared_ptr<ImageView> build() {
        return m_view;
    }

private:
    std::shared_ptr<ImageView> m_view;
};

} // namespace miqu
