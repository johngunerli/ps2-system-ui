#pragma once

#include <OpenGL/gl.h>
#include <string>
#include <unordered_map>
#include <cstdint>

enum class TextAlign {
    LEFT,
    CENTER,
    RIGHT
};

class TextRenderer {
public:
    static TextRenderer& instance();

    void init(float dpiScale = 1.0f);
    void setDpiScale(float scale);
    void clearCache();

    void drawText(
        const std::string& text,
        float x,
        float y,
        float fontSize,
        uint32_t color,
        TextAlign align = TextAlign::LEFT,
        bool bold = true,
        bool glow = false
    );

    float measureTextWidth(const std::string& text, float fontSize, bool bold = true);

    static void drawRoundedRect(
        float x, float y, float w, float h, float radius,
        uint32_t topColor, uint32_t bottomColor
    );

    static void drawRoundedRectOutline(
        float x, float y, float w, float h, float radius,
        float thickness, uint32_t color
    );

    static void drawGlowingPill(
        float x, float y, float w, float h,
        uint32_t bgColor, uint32_t borderColor, float glowAlpha = 0.4f
    );

    static void drawButtonBadge(
        const std::string& label,
        float cx, float cy, float radius,
        uint32_t badgeColor, uint32_t textColor
    );

private:
    TextRenderer() = default;
    ~TextRenderer();

    struct CachedText {
        GLuint textureId;
        int texWidth;
        int texHeight;
        float logicalWidth;
        float logicalHeight;
    };

    CachedText getOrCreateText(const std::string& text, float fontSize, bool bold, bool glow);

    float dpiScale = 2.0f;
    std::unordered_map<std::string, CachedText> cache;
};

