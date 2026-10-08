#pragma once

#include "miqutoolkit/view/view.hpp"
#include "miqutoolkit/view/image_view.hpp"
#include <string>
#include <memory>

namespace miqu {

class ListItemView : public View {
public:
    ListItemView() = default;
    ListItemView(std::string title, std::string subtitle = "", std::string icon_source = "");
    ~ListItemView() override = default;

    void set_title(std::string title) { m_title = std::move(title); }
    const std::string& get_title() const { return m_title; }

    void set_subtitle(std::string subtitle) { m_subtitle = std::move(subtitle); }
    const std::string& get_subtitle() const { return m_subtitle; }

    void set_trailing_text(std::string trailing) { m_trailing_text = std::move(trailing); }
    const std::string& get_trailing_text() const { return m_trailing_text; }

    void set_icon_source(std::string source) { m_icon_source = std::move(source); }
    const std::string& get_icon_source() const { return m_icon_source; }

    void set_icon_size(int size) { m_icon_size = size; }
    int get_icon_size() const { return m_icon_size; }

    void set_icon_corner_radius(int radius) { m_icon_radius = radius; }
    int get_icon_corner_radius() const { return m_icon_radius; }

    void set_is_image(bool is_image) { m_is_image = is_image; }
    bool is_image() const { return m_is_image; }

    void set_quality_mode(ImageQuality quality) { m_quality = quality; }
    ImageQuality get_quality_mode() const { return m_quality; }

    void set_highlight_subtitle(bool highlight) { m_highlight_subtitle = highlight; }
    bool is_subtitle_highlighted() const { return m_highlight_subtitle; }

    void set_highlight_trailing(bool highlight) { m_highlight_trailing = highlight; }
    bool is_trailing_highlighted() const { return m_highlight_trailing; }

    void draw(cairo_t* cr, const Rect& bounds) override;

private:
    std::string m_title;
    std::string m_subtitle;
    std::string m_trailing_text;
    std::string m_icon_source;
    int m_icon_size = 28;
    int m_icon_radius = 4;
    bool m_is_image = false;
    ImageQuality m_quality = ImageQuality::FullOriginal;
    bool m_highlight_subtitle = false;
    bool m_highlight_trailing = false;
};

class ListItemViewBuilder : public std::enable_shared_from_this<ListItemViewBuilder> {
public:
    static std::shared_ptr<ListItemViewBuilder> create() {
        return std::make_shared<ListItemViewBuilder>();
    }

    ListItemViewBuilder() : m_view(std::make_shared<ListItemView>()) {}

    std::shared_ptr<ListItemViewBuilder> title(std::string title) {
        m_view->set_title(std::move(title));
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> subtitle(std::string subtitle) {
        m_view->set_subtitle(std::move(subtitle));
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> trailingText(std::string trailing) {
        m_view->set_trailing_text(std::move(trailing));
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> iconSource(std::string src) {
        m_view->set_icon_source(std::move(src));
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> iconSize(int size) {
        m_view->set_icon_size(size);
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> iconCornerRadius(int radius) {
        m_view->set_icon_corner_radius(radius);
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> isImage(bool is_img) {
        m_view->set_is_image(is_img);
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> qualityMode(ImageQuality q) {
        m_view->set_quality_mode(q);
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> highlightSubtitle(bool hl) {
        m_view->set_highlight_subtitle(hl);
        return shared_from_this();
    }

    std::shared_ptr<ListItemViewBuilder> highlightTrailing(bool hl) {
        m_view->set_highlight_trailing(hl);
        return shared_from_this();
    }

    std::shared_ptr<ListItemView> build() {
        return m_view;
    }

private:
    std::shared_ptr<ListItemView> m_view;
};

} // namespace miqu
