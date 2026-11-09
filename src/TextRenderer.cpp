#include "TextRenderer.hpp"
#include <CoreGraphics/CoreGraphics.h>
#include <CoreText/CoreText.h>
#include <CoreFoundation/CoreFoundation.h>
#include <cmath>
#include <vector>
#include <algorithm>

constexpr float PI_VAL = 3.14159265358979323846f;

TextRenderer& TextRenderer::instance() {
    static TextRenderer inst;
    return inst;
}

TextRenderer::~TextRenderer() {
    clearCache();
}

void TextRenderer::init(float scale) {
    dpiScale = std::max(1.0f, scale);
}

void TextRenderer::setDpiScale(float scale) {
    float newScale = std::max(1.0f, scale);
    if (std::fabs(newScale - dpiScale) > 0.05f) {
        dpiScale = newScale;
        clearCache();
    }
}

void TextRenderer::clearCache() {
    for (auto& kv : cache) {
        if (kv.second.textureId != 0) {
            glDeleteTextures(1, &kv.second.textureId);
        }
    }
    cache.clear();
}

TextRenderer::CachedText TextRenderer::getOrCreateText(
    const std::string& text, float fontSize, bool bold, bool glow
) {
    if (text.empty()) {
        return {0, 0, 0, 0.0f, 0.0f};
    }

    int sizeKey = static_cast<int>(std::round(fontSize * 2.0f));
    std::string key = text + "|" + std::to_string(sizeKey) + "|" + (bold ? "B" : "R") + (glow ? "G" : "N");

    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    if (cache.size() > 500) {
        clearCache();
    }

    float scaledFontSize = fontSize * dpiScale;
    float pad = (glow ? 9.0f : 4.0f) * dpiScale;

    CFStringRef fontName = bold ? CFSTR("HelveticaNeue-Medium") : CFSTR("HelveticaNeue");
    CTFontRef font = CTFontCreateWithName(fontName, scaledFontSize, nullptr);
    if (!font) {
        font = CTFontCreateWithName(CFSTR("Helvetica-Bold"), scaledFontSize, nullptr);
    }

    CFStringRef cfText = CFStringCreateWithCString(kCFAllocatorDefault, text.c_str(), kCFStringEncodingUTF8);
    if (!cfText) {
        if (font) CFRelease(font);
        return {0, 0, 0, 0.0f, 0.0f};
    }

    CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
    CGFloat whiteComponents[4] = {1.0, 1.0, 1.0, 1.0};
    CGColorRef whiteColor = CGColorCreate(colorSpace, whiteComponents);

    const void* keys[] = { kCTFontAttributeName, kCTForegroundColorAttributeName };
    const void* values[] = { font, whiteColor };
    CFDictionaryRef attrs = CFDictionaryCreate(
        kCFAllocatorDefault, keys, values, 2,
        &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks
    );

    CFAttributedStringRef attrStr = CFAttributedStringCreate(kCFAllocatorDefault, cfText, attrs);
    CTLineRef line = CTLineCreateWithAttributedString(attrStr);

    CGFloat ascent = 0, descent = 0, leading = 0;
    double lineWidth = CTLineGetTypographicBounds(line, &ascent, &descent, &leading);

    int texW = std::max(4, static_cast<int>(std::ceil(lineWidth + pad * 2.0f)));
    int texH = std::max(4, static_cast<int>(std::ceil(ascent + descent + pad * 2.0f)));

    std::vector<uint8_t> pixels(texW * texH * 4, 0);

    CGContextRef ctx = CGBitmapContextCreate(
        pixels.data(), texW, texH, 8, texW * 4, colorSpace,
        static_cast<uint32_t>(kCGImageAlphaPremultipliedLast) | static_cast<uint32_t>(kCGBitmapByteOrder32Big)
    );

    if (ctx) {
        CGContextSetAllowsAntialiasing(ctx, true);
        CGContextSetShouldAntialias(ctx, true);
        CGContextSetAllowsFontSmoothing(ctx, true);
        CGContextSetShouldSmoothFonts(ctx, true);

        if (glow) {
            CGFloat glowComponents[4] = {0.45, 0.75, 1.0, 0.90};
            CGColorRef glowCol = CGColorCreate(colorSpace, glowComponents);
            CGContextSetShadowWithColor(ctx, CGSizeMake(0, 0), 6.0 * dpiScale, glowCol);
            CGColorRelease(glowCol);
        } else {
            CGFloat shadowComponents[4] = {0.0, 0.0, 0.05, 0.85};
            CGColorRef shadowCol = CGColorCreate(colorSpace, shadowComponents);
            CGContextSetShadowWithColor(ctx, CGSizeMake(1.5 * dpiScale, -1.5 * dpiScale), 2.5 * dpiScale, shadowCol);
            CGColorRelease(shadowCol);
        }

        CGContextSetTextPosition(ctx, pad, pad + descent);
        CTLineDraw(line, ctx);
        CGContextRelease(ctx);
    }

    GLuint texId = 0;
    glGenTextures(1, &texId);
    glBindTexture(GL_TEXTURE_2D, texId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texW, texH, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
    glBindTexture(GL_TEXTURE_2D, 0);

    CFRelease(line);
    CFRelease(attrStr);
    CFRelease(attrs);
    CGColorRelease(whiteColor);
    CGColorSpaceRelease(colorSpace);
    CFRelease(cfText);
    CFRelease(font);

    CachedText entry{
        texId,
        texW,
        texH,
        static_cast<float>(texW) / dpiScale,
        static_cast<float>(texH) / dpiScale
    };
    cache[key] = entry;
    return entry;
}

float TextRenderer::measureTextWidth(const std::string& text, float fontSize, bool bold) {
    if (text.empty()) return 0.0f;
    CachedText ct = getOrCreateText(text, fontSize, bold, false);
    float pad = 4.0f;
    return std::max(0.0f, ct.logicalWidth - pad * 2.0f);
}

void TextRenderer::drawText(
    const std::string& text,
    float x,
    float y,
    float fontSize,
    uint32_t color,
    TextAlign align,
    bool bold,
    bool glow
) {
    if (text.empty()) return;

    CachedText ct = getOrCreateText(text, fontSize, bold, glow);
    if (ct.textureId == 0) return;

    float pad = glow ? 9.0f : 4.0f;
    float drawX = x - pad;
    if (align == TextAlign::CENTER) {
        drawX = x - ct.logicalWidth * 0.5f;
    } else if (align == TextAlign::RIGHT) {
        drawX = x - ct.logicalWidth + pad;
    }
    float drawY = y - pad * 0.6f;

    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;
    float a = ((color >> 24) & 0xFF) / 255.0f;
    if (((color >> 24) & 0xFF) == 0) a = 1.0f;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, ct.textureId);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r * a, g * a, b * a, a);

    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(drawX, drawY);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(drawX + ct.logicalWidth, drawY);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(drawX + ct.logicalWidth, drawY + ct.logicalHeight);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(drawX, drawY + ct.logicalHeight);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

static void unpackColor(uint32_t col, float& r, float& g, float& b, float& a) {
    r = ((col >> 16) & 0xFF) / 255.0f;
    g = ((col >> 8) & 0xFF) / 255.0f;
    b = (col & 0xFF) / 255.0f;
    a = ((col >> 24) & 0xFF) / 255.0f;
    if (((col >> 24) & 0xFF) == 0) a = 1.0f;
}

static void generateRoundedPolygon(
    float x, float y, float w, float h, float radius,
    std::vector<std::pair<float, float>>& pts
) {
    float r = std::min({radius, w * 0.5f, h * 0.5f});
    const int segs = 10;

    auto addArc = [&](float cx, float cy, float startAngle, float endAngle) {
        for (int i = 0; i <= segs; ++i) {
            float t = static_cast<float>(i) / segs;
            float ang = startAngle + (endAngle - startAngle) * t;
            pts.push_back({cx + std::cos(ang) * r, cy + std::sin(ang) * r});
        }
    };

    addArc(x + w - r, y + r,     -PI_VAL * 0.5f, 0.0f);
    addArc(x + w - r, y + h - r,  0.0f,          PI_VAL * 0.5f);
    addArc(x + r,     y + h - r,  PI_VAL * 0.5f, PI_VAL);
    addArc(x + r,     y + r,      PI_VAL,        PI_VAL * 1.5f);
}

void TextRenderer::drawRoundedRect(
    float x, float y, float w, float h, float radius,
    uint32_t topColor, uint32_t bottomColor
) {
    float tr, tg, tb, ta;
    float br, bg, bb, ba;
    unpackColor(topColor, tr, tg, tb, ta);
    unpackColor(bottomColor, br, bg, bb, ba);

    std::vector<std::pair<float, float>> pts;
    generateRoundedPolygon(x, y, w, h, radius, pts);

    float cx = x + w * 0.5f;
    float cy = y + h * 0.5f;

    glBegin(GL_TRIANGLE_FAN);
    glColor4f((tr + br) * 0.5f, (tg + bg) * 0.5f, (tb + bb) * 0.5f, (ta + ba) * 0.5f);
    glVertex2f(cx, cy);

    for (size_t i = 0; i <= pts.size(); ++i) {
        const auto& p = pts[i % pts.size()];
        float ty = std::clamp((p.second - y) / std::max(1.0f, h), 0.0f, 1.0f);
        glColor4f(
            tr + (br - tr) * ty,
            tg + (bg - tg) * ty,
            tb + (bb - tb) * ty,
            ta + (ba - ta) * ty
        );
        glVertex2f(p.first, p.second);
    }
    glEnd();
}

void TextRenderer::drawRoundedRectOutline(
    float x, float y, float w, float h, float radius,
    float thickness, uint32_t color
) {
    float r, g, b, a;
    unpackColor(color, r, g, b, a);

    std::vector<std::pair<float, float>> outerPts;
    std::vector<std::pair<float, float>> innerPts;
    generateRoundedPolygon(x, y, w, h, radius, outerPts);
    generateRoundedPolygon(
        x + thickness, y + thickness,
        std::max(1.0f, w - thickness * 2.0f),
        std::max(1.0f, h - thickness * 2.0f),
        std::max(0.0f, radius - thickness),
        innerPts
    );

    glColor4f(r, g, b, a);
    glBegin(GL_TRIANGLE_STRIP);
    for (size_t i = 0; i <= outerPts.size(); ++i) {
        size_t idx = i % outerPts.size();
        glVertex2f(outerPts[idx].first, outerPts[idx].second);
        glVertex2f(innerPts[idx].first, innerPts[idx].second);
    }
    glEnd();
}

void TextRenderer::drawGlowingPill(
    float x, float y, float w, float h,
    uint32_t bgColor, uint32_t borderColor, float glowAlpha
) {
    float radius = h * 0.5f;

    if (glowAlpha > 0.01f) {
        uint32_t glowCol = (static_cast<uint32_t>(glowAlpha * 90.0f) << 24) | (borderColor & 0x00FFFFFF);
        drawRoundedRect(x - 3.0f, y - 3.0f, w + 6.0f, h + 6.0f, radius + 3.0f, glowCol, glowCol);
    }

    drawRoundedRect(x, y, w, h, radius, bgColor, bgColor);
    drawRoundedRect(x + 4.0f, y + 2.0f, w - 8.0f, h * 0.42f, radius * 0.4f, 0x28FFFFFF, 0x04FFFFFF);
    drawRoundedRectOutline(x, y, w, h, radius, 1.8f, borderColor);
}

void TextRenderer::drawButtonBadge(
    const std::string& label,
    float cx, float cy, float radius,
    uint32_t badgeColor, uint32_t textColor
) {
    float r = radius;
    drawRoundedRect(cx - r + 1.0f, cy - r + 2.0f, r * 2.0f, r * 2.0f, r, 0x88000000, 0x88000000);
    drawRoundedRect(cx - r, cy - r, r * 2.0f, r * 2.0f, r, badgeColor, badgeColor);
    drawRoundedRect(cx - r * 0.65f, cy - r * 0.8f, r * 1.3f, r * 0.7f, r * 0.35f, 0x55FFFFFF, 0x05FFFFFF);
    drawRoundedRectOutline(cx - r, cy - r, r * 2.0f, r * 2.0f, r, 1.5f, 0xAAFFFFFF);

    TextRenderer::instance().drawText(label, cx, cy - r * 0.58f, r * 1.15f, textColor, TextAlign::CENTER, true, false);
}
