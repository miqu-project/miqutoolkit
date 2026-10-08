#pragma once

#include <cstdint>
#include <string>
#include <algorithm>

namespace miqu {

struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;

    Color() = default;
    constexpr Color(float r_, float g_, float b_, float a_ = 1.0f) : r(r_), g(g_), b(b_), a(a_) {}

    static constexpr Color rgba(float r, float g, float b, float a = 1.0f) {
        return Color(r, g, b, a);
    }

    static constexpr Color rgb(float r, float g, float b) {
        return Color(r, g, b, 1.0f);
    }

    static constexpr Color transparent() {
        return Color(0.0f, 0.0f, 0.0f, 0.0f);
    }

    Color with_alpha(float new_alpha) const {
        return Color(r, g, b, std::clamp(new_alpha, 0.0f, 1.0f));
    }

    Color mix(const Color& other, float factor) const {
        factor = std::clamp(factor, 0.0f, 1.0f);
        return Color(
            r * (1.0f - factor) + other.r * factor,
            g * (1.0f - factor) + other.g * factor,
            b * (1.0f - factor) + other.b * factor,
            a * (1.0f - factor) + other.a * factor
        );
    }

    Color brighten(float factor) const {
        return mix(Color(1.0f, 1.0f, 1.0f, a), factor);
    }

    Color darken(float factor) const {
        return mix(Color(0.0f, 0.0f, 0.0f, a), factor);
    }

    static Color from_hex(const std::string& hex, const Color& fallback) {
        std::string s = hex;
        // Strip surrounding whitespace and quotes
        while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '"' || s.front() == '\'')) {
            s.erase(0, 1);
        }
        while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '"' || s.back() == '\'')) {
            s.pop_back();
        }
        // Strip trailing comment if present
        size_t comment = s.find('#', 1);
        if (comment != std::string::npos) {
            s = s.substr(0, comment);
            while (!s.empty() && (s.back() == ' ' || s.back() == '\t')) s.pop_back();
        }

        if (s.empty()) return fallback;
        if (s[0] == '#') s = s.substr(1);

        try {
            if (s.length() == 6) {
                uint32_t val = std::stoul(s, nullptr, 16);
                return Color(
                    ((val >> 16) & 0xFF) / 255.0f,
                    ((val >> 8) & 0xFF) / 255.0f,
                    (val & 0xFF) / 255.0f,
                    1.0f
                );
            } else if (s.length() == 8) {
                uint32_t val = std::stoul(s, nullptr, 16);
                return Color(
                    ((val >> 24) & 0xFF) / 255.0f,
                    ((val >> 16) & 0xFF) / 255.0f,
                    ((val >> 8) & 0xFF) / 255.0f,
                    (val & 0xFF) / 255.0f
                );
            } else if (s.length() == 3) {
                uint32_t val = std::stoul(s, nullptr, 16);
                return Color(
                    (((val >> 8) & 0xF) * 17) / 255.0f,
                    (((val >> 4) & 0xF) * 17) / 255.0f,
                    ((val & 0xF) * 17) / 255.0f,
                    1.0f
                );
            } else if (s.length() == 4) {
                uint32_t val = std::stoul(s, nullptr, 16);
                return Color(
                    (((val >> 12) & 0xF) * 17) / 255.0f,
                    (((val >> 8) & 0xF) * 17) / 255.0f,
                    (((val >> 4) & 0xF) * 17) / 255.0f,
                    ((val & 0xF) * 17) / 255.0f
                );
            }
        } catch (...) {
            return fallback;
        }
        return fallback;
    }

    static Color from_hex(const std::string& hex) {
        return from_hex(hex, Color(0.0f, 0.0f, 0.0f, 1.0f));
    }
};

} // namespace miqu
