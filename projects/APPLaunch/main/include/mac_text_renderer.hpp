#pragma once

#include <cstdint>
#include <string>

struct MacTextRenderStyle {
    uint8_t r = 255;
    uint8_t g = 255;
    uint8_t b = 255;
    uint8_t a = 255;
    float point_size = 14.0f;
    bool bold = false;
    bool center = false;
    bool wrap = false;
};

bool render_mac_text_png(const std::string &path,
                         const std::string &text,
                         int width,
                         int height,
                         const MacTextRenderStyle &style);