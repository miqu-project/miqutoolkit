#pragma once

#include <cstdint>
#include <string>

namespace miqu {

enum class MouseButton {
    Left = 272,
    Right = 273,
    Middle = 274,
    Back = 277,
    Forward = 278,
};

enum class PointerShape {
    Default,
    Pointer,
    Text,
    Grab,
    Grabbing,
    ResizeHorizontal,
    ResizeVertical,
};

enum class KeyboardModifier : uint32_t {
    None = 0,
    Shift = 1 << 0,
    Caps = 1 << 1,
    Control = 1 << 2,
    Alt = 1 << 3,
    Mod2 = 1 << 4,
    Mod3 = 1 << 5,
    Super = 1 << 6,
    Mod5 = 1 << 7,
};

inline KeyboardModifier operator|(KeyboardModifier a, KeyboardModifier b) {
    return static_cast<KeyboardModifier>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline bool operator&(KeyboardModifier a, KeyboardModifier b) {
    return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
}

struct KeyPressEvent {
    uint32_t keysym = 0;
    uint32_t raw_code = 0;
    uint32_t modifiers = 0;
    bool pressed = true;
    std::string utf8_text;

    bool has_ctrl() const { return (modifiers & static_cast<uint32_t>(KeyboardModifier::Control)) != 0; }
    bool has_alt() const { return (modifiers & static_cast<uint32_t>(KeyboardModifier::Alt)) != 0; }
    bool has_shift() const { return (modifiers & static_cast<uint32_t>(KeyboardModifier::Shift)) != 0; }
    bool has_super() const { return (modifiers & static_cast<uint32_t>(KeyboardModifier::Super)) != 0; }
};

enum class TouchPhase {
    Down,
    Motion,
    Up,
    Cancel,
};

struct TouchEvent {
    int32_t id = 0;
    double x = 0.0;
    double y = 0.0;
    TouchPhase phase = TouchPhase::Down;
    uint32_t time = 0;
};

} // namespace miqu

