#pragma once

#include <memory>

namespace miqu {

class View;

enum class LayoutDimension {
    MatchParent = -1,
    WrapContent = -2,
};

enum class Orientation {
    Horizontal,
    Vertical,
};

enum class Gravity {
    None = 0,
    Left = 1 << 0,
    Top = 1 << 1,
    Right = 1 << 2,
    Bottom = 1 << 3,
    CenterHorizontal = 1 << 4,
    CenterVertical = 1 << 5,
    Center = CenterHorizontal | CenterVertical,
    Start = Left,
    End = Right,
};

inline Gravity operator|(Gravity a, Gravity b) {
    return static_cast<Gravity>(static_cast<int>(a) | static_cast<int>(b));
}

inline bool operator&(Gravity a, Gravity b) {
    return (static_cast<int>(a) & static_cast<int>(b)) != 0;
}

struct LayoutParams {
    int width = static_cast<int>(LayoutDimension::WrapContent);
    int height = static_cast<int>(LayoutDimension::WrapContent);
    float weight = 0.0f;
    Gravity gravity = Gravity::None;

    // RelativeLayout rules
    bool align_parent_top = false;
    bool align_parent_bottom = false;
    bool align_parent_start = false;
    bool align_parent_end = false;
    bool center_in_parent = false;
    bool center_horizontal = false;
    bool center_vertical = false;

    std::shared_ptr<View> above;
    std::shared_ptr<View> below;
    std::shared_ptr<View> to_start_of;
    std::shared_ptr<View> to_end_of;

    LayoutParams() = default;
    LayoutParams(int w, int h) : width(w), height(h) {}
    LayoutParams(int w, int h, float wt) : width(w), height(h), weight(wt) {}
    LayoutParams(float wt) : width(static_cast<int>(LayoutDimension::MatchParent)), height(static_cast<int>(LayoutDimension::WrapContent)), weight(wt) {}
    LayoutParams(Gravity g) : gravity(g) {}
    LayoutParams(int w, int h, Gravity g) : width(w), height(h), gravity(g) {}
};

} // namespace miqu
