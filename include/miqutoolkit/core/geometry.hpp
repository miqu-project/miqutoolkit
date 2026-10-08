#pragma once

#include <algorithm>

namespace miqu {

struct Point {
    int x = 0;
    int y = 0;

    constexpr Point() = default;
    constexpr Point(int x_, int y_) : x(x_), y(y_) {}
};

struct Size {
    int width = 0;
    int height = 0;

    constexpr Size() = default;
    constexpr Size(int w, int h) : width(w), height(h) {}
};

struct Rect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    constexpr Rect() = default;
    constexpr Rect(int x_, int y_, int w, int h) : x(x_), y(y_), width(w), height(h) {}

    bool contains(int px, int py) const {
        return px >= x && px < (x + width) && py >= y && py < (y + height);
    }

    bool contains(const Point& pt) const {
        return contains(pt.x, pt.y);
    }
};

struct Padding {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    constexpr Padding() = default;
    constexpr Padding(int uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    constexpr Padding(int horizontal, int vertical) : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Padding(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
};

struct Margin {
    int left = 0;
    int top = 0;
    int right = 0;
    int bottom = 0;

    constexpr Margin() = default;
    constexpr Margin(int uniform) : left(uniform), top(uniform), right(uniform), bottom(uniform) {}
    constexpr Margin(int horizontal, int vertical) : left(horizontal), top(vertical), right(horizontal), bottom(vertical) {}
    constexpr Margin(int l, int t, int r, int b) : left(l), top(t), right(r), bottom(b) {}
};

} // namespace miqu
