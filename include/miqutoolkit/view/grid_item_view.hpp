#pragma once

#include "miqutoolkit/view/view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <string>
#include <memory>

namespace miqu {

class GridItemView : public View {
public:
    GridItemView() = default;
    GridItemView(std::string title, std::string icon_source, bool is_image = false);
    ~GridItemView() override;

    void set_title(std::string title);
    const std::string& get_title() const { return m_title; }

    void set_subtitle(std::string subtitle);
    const std::string& get_subtitle() const { return m_subtitle; }

    void set_icon_source(std::string source);
    const std::string& get_icon_source() const { return m_icon_source; }

    void set_is_image(bool is_image);
    bool is_image() const { return m_is_image; }

    void set_quality_mode(ImageQuality quality);
    ImageQuality get_quality_mode() const { return m_quality; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    void set_title_bold(bool bold);
    bool is_title_bold() const { return m_title_bold; }

    void set_highlight_title(bool highlight);
    bool is_title_highlighted() const { return m_highlight_title; }

    void set_highlight_subtitle(bool highlight) { m_highlight_subtitle = highlight; }
    bool is_subtitle_highlighted() const { return m_highlight_subtitle; }

    void draw(cairo_t* cr, const Rect& bounds) override;

private:
    std::string m_title;
    std::string m_subtitle;
    std::string m_icon_source;
    bool m_is_image = false;
    ImageQuality m_quality = ImageQuality::FullOriginal;
    int m_corner_radius = 6;
    bool m_title_bold = false;
    bool m_highlight_title = false;
    bool m_highlight_subtitle = false;

    // Retained Pango & View caches for instant redraws
    void* m_title_layout = nullptr; // PangoLayout*
    void* m_sub_layout = nullptr;   // PangoLayout*
    int m_last_text_max_w = -1;
    int m_cached_title_w = 0;
    int m_cached_title_h = 0;
    int m_cached_sub_w = 0;
    int m_cached_sub_h = 0;
    bool m_title_dirty = true;
    bool m_sub_dirty = true;
    ImageView m_icon_view;
};

class GridItemViewBuilder : public std::enable_shared_from_this<GridItemViewBuilder> {
public:
    static std::shared_ptr<GridItemViewBuilder> create() {
        return std::make_shared<GridItemViewBuilder>();
    }

    GridItemViewBuilder() : m_view(std::make_shared<GridItemView>()) {}

    std::shared_ptr<GridItemViewBuilder> title(std::string title) {
        m_view->set_title(std::move(title));
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> subtitle(std::string sub) {
        m_view->set_subtitle(std::move(sub));
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> iconSource(std::string src) {
        m_view->set_icon_source(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> isImage(bool is_img) {
        m_view->set_is_image(is_img);
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> qualityMode(ImageQuality q) {
        m_view->set_quality_mode(q);
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> cornerRadius(int r) {
        m_view->set_corner_radius(r);
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> titleBold(bool bold) {
        m_view->set_title_bold(bold);
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> highlightTitle(bool hl) {
        m_view->set_highlight_title(hl);
        return shared_from_this();
    }

    std::shared_ptr<GridItemViewBuilder> highlightSubtitle(bool hl) {
        m_view->set_highlight_subtitle(hl);
        return shared_from_this();
    }

    std::shared_ptr<GridItemView> build() {
        return m_view;
    }

private:
    std::shared_ptr<GridItemView> m_view;
};

} // namespace miqu
