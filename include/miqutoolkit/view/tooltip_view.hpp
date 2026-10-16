#pragma once

#include "miqutoolkit/view/view.hpp"
#include "miqutoolkit/core/color.hpp"
#include <string>

namespace miqu {

/**
 * @brief Styled tooltip bubble presenting concise context information for interactive views.
 * Adheres to Miqu design language tokens: rounded corners, elevated ambient drop shadow,
 * surface_variant container, and typography sizing.
 */
class TooltipView : public View {
public:
    TooltipView();
    explicit TooltipView(std::string text, std::string shortcut = "");
    ~TooltipView() override = default;

    void set_text(std::string text);
    const std::string& get_text() const { return m_text; }

    void set_shortcut(std::string shortcut);
    const std::string& get_shortcut() const { return m_shortcut; }

    void set_corner_radius(int radius) { m_corner_radius = radius; }
    int get_corner_radius() const { return m_corner_radius; }

    void set_elevation(double elevation) { m_elevation = elevation; }
    double get_elevation() const { return m_elevation; }

    Size measure_size() const override;
    Size measure_size(int avail_width) const override;
    void draw(cairo_t* cr, const Rect& bounds) override;

private:
    std::string m_text;
    std::string m_shortcut;
    Padding m_padding{10, 5, 10, 5};
    int m_corner_radius = 6;
    double m_elevation = 3.0;
};

} // namespace miqu
