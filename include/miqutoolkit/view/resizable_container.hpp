#pragma once

#include "miqutoolkit/view/view_group.hpp"
#include <memory>

namespace miqu {

class ResizableContainer : public ViewGroup {
public:
    ResizableContainer();
    explicit ResizableContainer(std::shared_ptr<View> content);
    ~ResizableContainer() override = default;

    void set_content(std::shared_ptr<View> content);
    std::shared_ptr<View> get_content() const { return m_content; }

    void set_resizable(bool resizable) { m_resizable = resizable; }
    bool is_resizable() const { return m_resizable; }

    void set_active(bool active) { m_active = active; }
    bool is_active() const { return m_active; }

    void set_hover_resize(bool hover) { m_hover_resize = hover; }
    bool is_hover_resize() const { return m_hover_resize; }

    void set_active_stroke_color(const Color& color) { m_active_color = color; }
    const Color& get_active_stroke_color() const { return m_active_color; }

    void set_grip_size(int size) { m_grip_size = size; }
    int get_grip_size() const { return m_grip_size; }

    bool is_in_resize_grip(int lx, int ly, const Rect& bounds) const;

    void draw(cairo_t* cr, const Rect& bounds) override;
    bool on_mouse_button(int lx, int ly, MouseButton button, bool pressed, const Rect& bounds) override;
    bool on_mouse_move(int lx, int ly, const Rect& bounds) override;

private:
    std::shared_ptr<View> m_content;
    bool m_resizable = false;
    bool m_active = false;
    bool m_hover_resize = false;
    int m_grip_size = 24;
    Color m_active_color = Color::rgba(0.2f, 0.75f, 1.0f, 0.75f);
};

class ResizableContainerBuilder : public std::enable_shared_from_this<ResizableContainerBuilder> {
public:
    ResizableContainerBuilder() : m_container(std::make_shared<ResizableContainer>()) {}

    static std::shared_ptr<ResizableContainerBuilder> create() {
        return std::make_shared<ResizableContainerBuilder>();
    }

    std::shared_ptr<ResizableContainerBuilder> content(std::shared_ptr<View> view) {
        m_container->set_content(std::move(view));
        return shared_from_this();
    }

    std::shared_ptr<ResizableContainerBuilder> resizable(bool r = true) {
        m_container->set_resizable(r);
        return shared_from_this();
    }

    std::shared_ptr<ResizableContainerBuilder> active(bool a = true) {
        m_container->set_active(a);
        return shared_from_this();
    }

    std::shared_ptr<ResizableContainerBuilder> activeStrokeColor(const Color& c) {
        m_container->set_active_stroke_color(c);
        return shared_from_this();
    }

    std::shared_ptr<ResizableContainer> build() {
        return m_container;
    }

private:
    std::shared_ptr<ResizableContainer> m_container;
};

} // namespace miqu
