#include "Renderer.hpp"
#include "TextRenderer.hpp"
#include <cmath>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <iostream>

constexpr float PI = 3.14159265358979323846f;

Renderer::Renderer() {
    // Initialize the 7 Dancing Dots ("Seven Stars") of the PS2 BIOS
    const uint32_t starColors[7] = {
        0xFF88D8FF,
        0xFF68B8FF,
        0xFFA0E4FF,
        0xFF58A8FF,
        0xFF90DCFF,
        0xFF74C4FF,
        0xFFB0ECFF
    };

    sevenStars.resize(7);
    for (int i = 0; i < 7; ++i) {
        sevenStars[i].color = starColors[i];
        sevenStars[i].currentPos = {-1.35f, 0.05f, -5.5f};
    }

    // Populate PS2 Boot Monolith Towers for top-down 1-point perspective view:
    // (x, y) is the horizontal/vertical position around the center vanishing point,
    // width is the square cross-section size, height is how far the bar rises toward the top-down camera.
    bootTowers = {
        // Deep dim background squares (short towers deep in the mist)
        {  0.92f,  2.25f, 0.56f,  8.5f, false },
        { -2.55f,  0.35f, 0.50f,  8.2f, false },
        { -2.75f,  1.25f, 0.52f,  7.5f, false },
        { -1.85f, -0.52f, 0.48f, 11.2f, false },
        { -2.15f, -1.75f, 0.52f,  8.0f, false },
        {  0.12f, -2.05f, 0.50f,  7.8f, false },
        { -0.05f,  0.72f, 0.48f, 11.8f, false },
        { -1.42f,  0.42f, 0.54f, 12.6f, false },
        {  0.96f, -0.46f, 0.48f, 13.4f, false },

        // Medium-height outer & mid-ground bars
        {  2.85f,  1.62f, 0.56f, 15.8f, false },
        {  2.22f,  0.76f, 0.56f, 16.6f, false },
        { -2.32f, -2.42f, 0.56f, 17.2f, false },
        { -1.76f, -2.42f, 0.56f, 17.2f, false },
        { -0.86f, -1.16f, 0.54f, 17.5f, false },

        // Bright white-lit central square tower cap right behind the dark floating cube
        { -0.78f,  0.46f, 0.54f, 17.4f, false },

        // Tall foreground monolith bars splaying radially outward from the center
        { -0.36f,  2.35f, 0.58f, 19.3f, false }, // Top-center tall bar
        {  0.42f,  1.42f, 0.54f, 18.2f, false }, // Upper-center-right bar
        {  3.12f,  0.12f, 0.56f, 18.1f, false }, // Far-right middle bar
        {  1.72f,  0.04f, 0.56f, 18.8f, false }, // Mid-right bar (under "ment")
        {  0.58f, -0.02f, 0.54f, 18.4f, false }, // Center-right bar (under "Enter")
        {  1.56f, -1.24f, 0.56f, 18.6f, false }, // Lower-right bar
        {  3.02f, -2.12f, 0.58f, 18.3f, false }, // Far bottom-right bar
        { -3.22f, -1.26f, 0.58f, 18.9f, false }, // Far-left bar
        { -1.20f, -2.36f, 0.58f, 19.0f, false }, // Bottom-left tall bar

        // Prominent lower-left hollow-top monolith bar right below "Sony"
        { -1.02f, -0.68f, 0.62f, 19.6f, true  }
    };
}

Renderer::~Renderer() {}

void Renderer::init(int logicalW, int logicalH, int drawableW, int drawableH) {
    resize(logicalW, logicalH, drawableW, drawableH);

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);

    glClearColor(0.0f, 0.0f, 0.02f, 1.0f);
}

void Renderer::resize(int logicalW, int logicalH, int drawableW, int drawableH) {
    windowWidth = logicalW > 0 ? logicalW : 1280;
    windowHeight = logicalH > 0 ? logicalH : 720;
    fbWidth = drawableW > 0 ? drawableW : windowWidth;
    fbHeight = drawableH > 0 ? drawableH : windowHeight;
    dpiScale = static_cast<float>(fbWidth) / static_cast<float>(windowWidth);

    TextRenderer::instance().setDpiScale(dpiScale);
    glViewport(0, 0, fbWidth, fbHeight);
}

void Renderer::triggerDiscLaunchSpin() {
    discSpinSpeed = 840.0f;
}

int Renderer::hitTestMainMenu(int mouseX, int mouseY) const {
    float menuX = windowWidth * 0.57f;
    float startY = windowHeight * 0.43f;
    float spacing = 64.0f;

    if (mouseX >= menuX - 30.0f && mouseX <= menuX + 420.0f) {
        for (int i = 0; i < 2; ++i) {
            float iy = startY + i * spacing;
            if (mouseY >= iy - 12.0f && mouseY <= iy + 46.0f) {
                return i;
            }
        }
    }
    return -1;
}

int Renderer::hitTestBrowserDevices(int mouseX, int mouseY) const {
    if (mouseY < windowHeight * 0.25f || mouseY > windowHeight * 0.75f) return -1;
    float normX = static_cast<float>(mouseX) / static_cast<float>(windowWidth);
    if (normX >= 0.14f && normX < 0.32f) return 0;
    if (normX >= 0.32f && normX < 0.50f) return 1;
    if (normX >= 0.50f && normX < 0.68f) return 2;
    if (normX >= 0.68f && normX <= 0.86f) return 3;
    return -1;
}

int Renderer::hitTestBrowserGrid(int mouseX, int mouseY, size_t totalApps, size_t currentSelected) const {
    if (totalApps == 0) return -1;
    if (mouseY < 130 || mouseY > windowHeight - 130) return -1;

    int currentRow = static_cast<int>(currentSelected / 4);
    int topVisibleRow = std::max(0, currentRow - 1);

    float gridLeft = windowWidth * 0.18f;
    float gridRight = windowWidth * 0.82f;
    if (mouseX < gridLeft || mouseX > gridRight) return -1;

    int col = static_cast<int>(((mouseX - gridLeft) / (gridRight - gridLeft)) * 4.0f);
    col = std::clamp(col, 0, 3);

    float gridTop = 140.0f;
    float gridBot = windowHeight - 150.0f;
    int visRow = static_cast<int>(((mouseY - gridTop) / (gridBot - gridTop)) * 3.0f);
    visRow = std::clamp(visRow, 0, 2);

    size_t idx = static_cast<size_t>((topVisibleRow + visRow) * 4 + col);
    if (idx < totalApps) {
        return static_cast<int>(idx);
    }
    return -1;
}

int Renderer::hitTestSystemConfig(int mouseX, int mouseY) const {
    float menuX = std::min(windowWidth * 0.46f, windowWidth - 540.0f);
    float startY = 148.0f;
    float spacing = 64.0f;
    if (mouseX >= menuX - 20.0f && mouseX <= windowWidth - 60.0f) {
        for (int i = 0; i < 6; ++i) {
            float iy = startY + i * spacing;
            if (mouseY >= iy - 10.0f && mouseY <= iy + 48.0f) {
                return i;
            }
        }
    }
    return -1;
}

// Exact PS2 Dancing Dots Mathematical Model:
// 1. A circular curve of radius R in local XY space: (R*cos(theta), R*sin(theta), 0)
// 2. The circle plane rotates in 3D like a spinning/wobbling coin on a table
//    (precessing normal axis + nutation tilt + vertical/depth bobbing)
Vec3 Renderer::evaluateOrbCirclePoint(float theta, float t, const Vec3& center, float radiusScale) const {
    float R = 0.92f * radiusScale;

    // Point on the circular curve in local plane coordinates
    float lx = R * std::cos(theta);
    float ly = R * std::sin(theta);
    float lz = 0.0f;

    // Coin-on-a-table precession angles:
    // phi: direction of tilt precessing around the table
    // tilt: angle of the coin plane relative to the camera/table (oscillates between edge-on and angled)
    float phi  = t * 1.35f;
    float tilt = 1.08f + 0.36f * std::sin(t * 0.78f); // ~62 deg +/- 20 deg so we see both wide loops and tight coin edges

    // Step A: Rotate in plane by -phi
    float cPhi = std::cos(-phi);
    float sPhi = std::sin(-phi);
    float x1 = lx * cPhi - ly * sPhi;
    float y1 = lx * sPhi + ly * cPhi;
    float z1 = lz;

    // Step B: Tilt the coin plane around the X axis by `tilt`
    float cTilt = std::cos(tilt);
    float sTilt = std::sin(tilt);
    float x2 = x1;
    float y2 = y1 * cTilt - z1 * sTilt;
    float z2 = y1 * sTilt + z1 * cTilt;

    // Step C: Rotate back by +phi (completing the rolling-coin wobble without slipping) + slow Z bank
    float bank = phi + 0.45f * std::sin(t * 0.52f);
    float cBank = std::cos(bank);
    float sBank = std::sin(bank);
    float x3 = x2 * cBank - y2 * sBank;
    float y3 = x2 * sBank + y2 * cBank;
    float z3 = z2;

    // Step D: Parent curve bobbing up & down and breathing in/out in 3D space
    float bobX = 0.14f * std::sin(t * 0.92f);
    float bobY = 0.24f * std::sin(t * 1.28f) + 0.07f * std::cos(t * 2.15f);
    float bobZ = 0.22f * std::cos(t * 0.84f);

    return {
        center.x + bobX + x3,
        center.y + bobY + y3,
        center.z + bobZ + z3
    };
}

void Renderer::updateSimulations(float dt, ScreenState state, MenuTarget target) {
    (void)target;
    globalTime += dt;
    discSpinAngle += discSpinSpeed * dt;
    if (discSpinAngle > 360.0f) discSpinAngle -= 360.0f;
    discSpinSpeed += (110.0f - discSpinSpeed) * std::min(1.0f, dt * 2.5f);

    if (state == ScreenState::MAIN_MENU) {
        targetClusterCenter = {-1.38f, 0.05f, -5.4f};
    } else if (state == ScreenState::SYSTEM_CONFIG || state == ScreenState::VERSION_INFO) {
        targetClusterCenter = {-1.65f, 0.05f, -5.6f};
    } else if (state == ScreenState::BROWSER_DEVICES) {
        targetClusterCenter = {0.0f, 0.25f, -9.2f};
    } else {
        targetClusterCenter = {-3.0f, 1.6f, -10.0f};
    }

    float lerpRate = std::min(1.0f, dt * 4.5f);
    orbClusterCenter.x += (targetClusterCenter.x - orbClusterCenter.x) * lerpRate;
    orbClusterCenter.y += (targetClusterCenter.y - orbClusterCenter.y) * lerpRate;
    orbClusterCenter.z += (targetClusterCenter.z - orbClusterCenter.z) * lerpRate;

    // Recompute each of the 7 orbs along the parented circular curve,
    // plus 36 dense analytical sub-steps along the curve for silky-smooth circular trails!
    const int numTrailSteps = 36;
    const float trailDuration = 0.26f; // seconds of motion blur wake along the circular path

    for (int i = 0; i < 7; ++i) {
        auto& orb = sevenStars[i];
        orb.trail.clear();

        for (int step = 0; step < numTrailSteps; ++step) {
            float tSample = globalTime - (static_cast<float>(step) / numTrailSteps) * trailDuration;

            // Fanning / Un-fanning along the circular curve:
            // Each orb i moves at a slightly different speed as fanFactor oscillates between
            // bunched together (deltaTheta = 0.055 rad) and fanned evenly around the full circle (2*PI/7 rad)
            float fanWave = 0.5f - 0.5f * std::cos(tSample * 0.62f);
            // Smooth pause at tight cluster and full circle extremes
            float fanSmooth = fanWave * fanWave * (3.0f - 2.0f * fanWave);

            float minSpacing = 0.055f;               // Tightly bunched cluster on the rim
            float maxSpacing = (2.0f * PI) / 7.0f;   // Full 360-degree 7-star circle
            float deltaTheta = minSpacing + (maxSpacing - minSpacing) * fanSmooth;

            // Base orbital spin around the circular curve + slight speed harmonic
            float baseTheta = tSample * 3.85f + 0.35f * std::sin(tSample * 1.15f);
            float theta_i = baseTheta + (i - 3.0f) * deltaTheta;

            Vec3 pt = evaluateOrbCirclePoint(theta_i, tSample, orbClusterCenter, 1.0f);
            orb.trail.push_back(pt);
        }

        orb.currentPos = orb.trail.front();
    }

    gridScrollY += (targetGridScrollY - gridScrollY) * std::min(1.0f, dt * 9.0f);
}

void Renderer::setup3DProjection(float fovDegrees) {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight);
    float fov = fovDegrees * (PI / 180.0f);
    float nearVal = 0.1f;
    float farVal = 120.0f;
    float f = 1.0f / std::tan(fov / 2.0f);
    GLfloat m[16] = {
        f / aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, (farVal + nearVal) / (nearVal - farVal), -1,
        0, 0, (2.0f * farVal * nearVal) / (nearVal - farVal), 0
    };
    glLoadMatrixf(m);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}

void Renderer::renderPS2BackgroundVoid(ScreenState state, float bootProgress) {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    if (state == ScreenState::BOOT) {
        // Pure pitch-black outer void with concentrated central royal-blue nebula cloud
        glBegin(GL_QUADS);
        glColor4f(0.0f, 0.0f, 0.005f, 1.0f);
        glVertex2f(0, 0);
        glVertex2f(windowWidth, 0);
        glVertex2f(windowWidth, windowHeight);
        glVertex2f(0, windowHeight);
        glEnd();

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        float cx0 = windowWidth * 0.46f;
        float cy0 = windowHeight * 0.48f;
        float cloudAlpha = (bootProgress < 0.78f) ? 1.0f : std::max(0.35f, 1.0f - (bootProgress - 0.78f) * 2.5f);

        for (int layer = 0; layer < 8; ++layer) {
            float ang = layer * 1.15f + globalTime * 0.22f;
            float dist = (layer == 0) ? 0.0f : (38.0f + (layer % 3) * 34.0f);
            float cx = cx0 + std::cos(ang) * dist;
            float cy = cy0 + std::sin(ang) * dist * 0.82f;
            float rad = 155.0f + (layer % 4) * 52.0f;

            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.045f, 0.13f, 0.44f, 0.14f * cloudAlpha);
            glVertex2f(cx, cy);
            glColor4f(0.0f, 0.01f, 0.06f, 0.0f);
            for (int s = 0; s <= 36; ++s) {
                float a = (2.0f * PI * s) / 36.0f;
                glVertex2f(cx + std::cos(a) * rad * 1.15f, cy + std::sin(a) * rad);
            }
            glEnd();
        }
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        return;
    }

    glBegin(GL_QUADS);
    glColor4f(0.002f, 0.005f, 0.020f, 1.0f);
    glVertex2f(0, 0);
    glVertex2f(windowWidth, 0);
    glColor4f(0.012f, 0.032f, 0.095f, 1.0f);
    glVertex2f(windowWidth, windowHeight * 0.55f);
    glVertex2f(0, windowHeight * 0.55f);

    glColor4f(0.012f, 0.032f, 0.095f, 1.0f);
    glVertex2f(0, windowHeight * 0.55f);
    glVertex2f(windowWidth, windowHeight * 0.55f);
    glColor4f(0.002f, 0.004f, 0.015f, 1.0f);
    glVertex2f(windowWidth, windowHeight);
    glVertex2f(0, windowHeight);
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    float focusX = windowWidth * 0.34f;
    float focusY = windowHeight * 0.50f;
    float intensity = 1.0f;

    if (state == ScreenState::BROWSER_DEVICES || state == ScreenState::BROWSER_GRID) {
        focusX = windowWidth * 0.50f;
        intensity = 0.85f;
    }

    for (int layer = 0; layer < 5; ++layer) {
        float rad = (130.0f + layer * 95.0f) * (1.0f + 0.06f * std::sin(globalTime * 0.7f + layer));
        float cx = focusX + std::cos(globalTime * 0.30f + layer * 1.3f) * 32.0f;
        float cy = focusY + std::sin(globalTime * 0.40f + layer * 1.1f) * 24.0f;

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(0.06f * intensity, 0.18f * intensity, 0.55f * intensity, 0.075f);
        glVertex2f(cx, cy);
        glColor4f(0.0f, 0.02f, 0.12f, 0.0f);
        for (int s = 0; s <= 36; ++s) {
            float a = (2.0f * PI * s) / 36.0f;
            glVertex2f(cx + std::cos(a) * rad * 1.35f, cy + std::sin(a) * rad);
        }
        glEnd();
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

// Top-Down Monolith Bar:
// Camera looks straight down -Z into the abyss.
// Each bar stands at (x, y) and rises along +Z from zBase (-26.0) up to zTop (zBase + h),
// so its square top cap faces the viewer and its 4 side shafts splay radially outward from the center!
void Renderer::drawMonolithTower(float x, float y, float w, float h, float alpha, bool hollowTop) {
    float hw = w * 0.5f;
    float zBase = -26.0f;
    float zTop  = zBase + h;

    // Brightness depends on how high the bar rises out of the deep mist toward the camera
    float lit = std::clamp((zTop - (-20.5f)) / 13.8f, 0.10f, 1.0f);
    // Extra blue-white rim illumination for bars close to the central nebula
    float distCenter = std::sqrt(x * x + y * y);
    float centerGlow = std::clamp(1.22f - distCenter * 0.12f, 0.78f, 1.15f);
    float topB = std::min(1.0f, lit * centerGlow);

    // 1. Four perspective side walls receding from zTop down to zBase in the center mist
    float sTopR = 0.58f * topB;
    float sTopG = 0.62f * topB;
    float sTopB = 0.70f * topB;

    glBegin(GL_QUADS);
    // North wall (+Y)
    glColor4f(sTopR * 0.92f, sTopG * 0.92f, sTopB * 0.95f, alpha);
    glVertex3f(x - hw, y + hw, zTop);
    glVertex3f(x + hw, y + hw, zTop);
    glColor4f(0.01f, 0.02f, 0.06f, alpha);
    glVertex3f(x + hw, y + hw, zBase);
    glVertex3f(x - hw, y + hw, zBase);

    // South wall (-Y)
    glColor4f(sTopR * 0.78f, sTopG * 0.78f, sTopB * 0.82f, alpha);
    glVertex3f(x + hw, y - hw, zTop);
    glVertex3f(x - hw, y - hw, zTop);
    glColor4f(0.01f, 0.02f, 0.06f, alpha);
    glVertex3f(x - hw, y - hw, zBase);
    glVertex3f(x + hw, y - hw, zBase);

    // West wall (-X)
    glColor4f(sTopR * 0.84f, sTopG * 0.84f, sTopB * 0.88f, alpha);
    glVertex3f(x - hw, y - hw, zTop);
    glVertex3f(x - hw, y + hw, zTop);
    glColor4f(0.01f, 0.02f, 0.06f, alpha);
    glVertex3f(x - hw, y + hw, zBase);
    glVertex3f(x - hw, y - hw, zBase);

    // East wall (+X)
    glColor4f(sTopR * 1.02f, sTopG * 1.02f, sTopB * 1.05f, alpha);
    glVertex3f(x + hw, y + hw, zTop);
    glVertex3f(x + hw, y - hw, zTop);
    glColor4f(0.01f, 0.02f, 0.06f, alpha);
    glVertex3f(x + hw, y - hw, zBase);
    glVertex3f(x + hw, y + hw, zBase);
    glEnd();

    // Subtle longitudinal stone seam lines along the side shafts
    if (lit > 0.35f) {
        glLineWidth(1.0f * dpiScale);
        glBegin(GL_LINES);
        glColor4f(0.32f * topB, 0.36f * topB, 0.44f * topB, alpha * 0.55f);
        glVertex3f(x, y + hw + 0.002f, zTop);
        glColor4f(0.02f, 0.04f, 0.10f, 0.0f);
        glVertex3f(x, y + hw + 0.002f, zBase + 6.0f);

        glColor4f(0.32f * topB, 0.36f * topB, 0.44f * topB, alpha * 0.55f);
        glVertex3f(x, y - hw - 0.002f, zTop);
        glColor4f(0.02f, 0.04f, 0.10f, 0.0f);
        glVertex3f(x, y - hw - 0.002f, zBase + 6.0f);

        glColor4f(0.32f * topB, 0.36f * topB, 0.44f * topB, alpha * 0.55f);
        glVertex3f(x - hw - 0.002f, y, zTop);
        glColor4f(0.02f, 0.04f, 0.10f, 0.0f);
        glVertex3f(x - hw - 0.002f, y, zBase + 6.0f);

        glColor4f(0.32f * topB, 0.36f * topB, 0.44f * topB, alpha * 0.55f);
        glVertex3f(x + hw + 0.002f, y, zTop);
        glColor4f(0.02f, 0.04f, 0.10f, 0.0f);
        glVertex3f(x + hw + 0.002f, y, zBase + 6.0f);
        glEnd();
    }

    // 2. Square Top Cap facing directly at the top-down camera (z = zTop)
    float cR = 0.78f * topB;
    float cG = 0.82f * topB;
    float cB = 0.89f * topB;

    if (hollowTop) {
        // Outer stone rim + deep black recessed square interior (the lower-left pillar below "Sony")
        glBegin(GL_QUADS);
        glColor4f(cR * 0.85f, cG * 0.85f, cB * 0.88f, alpha);
        glVertex3f(x - hw, y - hw, zTop);
        glVertex3f(x + hw, y - hw, zTop);
        glVertex3f(x + hw, y + hw, zTop);
        glVertex3f(x - hw, y + hw, zTop);

        float iw = hw * 0.80f;
        glColor4f(0.01f, 0.015f, 0.03f, alpha);
        glVertex3f(x - iw, y - iw, zTop + 0.003f);
        glVertex3f(x + iw, y - iw, zTop + 0.003f);
        glVertex3f(x + iw, y + iw, zTop + 0.003f);
        glVertex3f(x - iw, y + iw, zTop + 0.003f);
        glEnd();
    } else {
        // 2x2 stone-block textured square top cap
        glBegin(GL_QUADS);
        // Top-left quadrant
        glColor4f(cR * 0.96f, cG * 0.96f, cB * 0.98f, alpha);
        glVertex3f(x - hw, y,      zTop);
        glVertex3f(x,      y,      zTop);
        glColor4f(cR * 1.04f, cG * 1.04f, cB * 1.05f, alpha);
        glVertex3f(x,      y + hw, zTop);
        glVertex3f(x - hw, y + hw, zTop);

        // Top-right quadrant
        glColor4f(cR * 1.02f, cG * 1.02f, cB * 1.04f, alpha);
        glVertex3f(x,      y,      zTop);
        glVertex3f(x + hw, y,      zTop);
        glColor4f(cR * 0.92f, cG * 0.92f, cB * 0.95f, alpha);
        glVertex3f(x + hw, y + hw, zTop);
        glVertex3f(x,      y + hw, zTop);

        // Bottom-left quadrant
        glColor4f(cR * 0.90f, cG * 0.90f, cB * 0.93f, alpha);
        glVertex3f(x - hw, y - hw, zTop);
        glVertex3f(x,      y - hw, zTop);
        glColor4f(cR * 0.98f, cG * 0.98f, cB * 1.00f, alpha);
        glVertex3f(x,      y,      zTop);
        glVertex3f(x - hw, y,      zTop);

        // Bottom-right quadrant
        glColor4f(cR * 0.95f, cG * 0.95f, cB * 0.97f, alpha);
        glVertex3f(x,      y - hw, zTop);
        glVertex3f(x + hw, y - hw, zTop);
        glColor4f(cR * 0.88f, cG * 0.88f, cB * 0.91f, alpha);
        glVertex3f(x + hw, y,      zTop);
        glVertex3f(x,      y,      zTop);
        glEnd();

        // Subtle stone block cross-seams and inner bevel on the square top cap
        if (lit > 0.25f) {
            float zSeam = zTop + 0.004f;
            glLineWidth(1.1f * dpiScale);
            glColor4f(cR * 0.68f, cG * 0.70f, cB * 0.76f, alpha * 0.65f);
            glBegin(GL_LINES);
            glVertex3f(x - hw * 0.92f, y, zSeam);
            glVertex3f(x + hw * 0.92f, y, zSeam);
            glVertex3f(x, y - hw * 0.92f, zSeam);
            glVertex3f(x, y + hw * 0.92f, zSeam);
            glEnd();

            float bw = hw * 0.74f;
            glColor4f(cR * 0.76f, cG * 0.78f, cB * 0.84f, alpha * 0.45f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(x - bw, y - bw, zSeam);
            glVertex3f(x + bw, y - bw, zSeam);
            glVertex3f(x + bw, y + bw, zSeam);
            glVertex3f(x - bw, y + bw, zSeam);
            glEnd();
        }
    }

    // Crisp outer rim highlight on the square top cap
    glLineWidth(1.2f * dpiScale);
    glColor4f(std::min(1.0f, cR * 1.12f), std::min(1.0f, cG * 1.12f), std::min(1.0f, cB * 1.15f), alpha * 0.75f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(x - hw, y - hw, zTop + 0.005f);
    glVertex3f(x + hw, y - hw, zTop + 0.005f);
    glVertex3f(x + hw, y + hw, zTop + 0.005f);
    glVertex3f(x - hw, y + hw, zTop + 0.005f);
    glEnd();
}

void Renderer::drawGlowBillboard(float x, float y, float z, float radius, uint32_t color, float alpha, float coreRatio) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    GLfloat mv[16];
    glGetFloatv(GL_MODELVIEW_MATRIX, mv);
    Vec3 right = {mv[0], mv[4], mv[8]};
    Vec3 up    = {mv[1], mv[5], mv[9]};

    const int segs = 24;
    // Soft Eevee-style outer emission bloom
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, alpha * 0.65f);
    glVertex3f(x, y, z);
    glColor4f(r * 0.5f, g * 0.7f, b, 0.0f);
    for (int i = 0; i <= segs; ++i) {
        float ang = (2.0f * PI * i) / segs;
        float ca = std::cos(ang) * radius;
        float sa = std::sin(ang) * radius;
        glVertex3f(
            x + right.x * ca + up.x * sa,
            y + right.y * ca + up.y * sa,
            z + right.z * ca + up.z * sa
        );
    }
    glEnd();

    // Crisp, small emissive white-cyan sphere core
    float coreR = radius * coreRatio;
    glBegin(GL_TRIANGLE_FAN);
    glColor4f(1.0f, 1.0f, 1.0f, std::min(1.0f, alpha * 1.25f));
    glVertex3f(x, y, z);
    glColor4f(r, g, b, 0.0f);
    for (int i = 0; i <= segs; ++i) {
        float ang = (2.0f * PI * i) / segs;
        float ca = std::cos(ang) * coreR;
        float sa = std::sin(ang) * coreR;
        glVertex3f(
            x + right.x * ca + up.x * sa,
            y + right.y * ca + up.y * sa,
            z + right.z * ca + up.z * sa
        );
    }
    glEnd();
}

void Renderer::drawVolumetricSpotlight(
    float x, float yTop, float yBot, float z,
    float topR, float botR, uint32_t color, float alpha
) {
    float r = ((color >> 16) & 0xFF) / 255.0f;
    float g = ((color >> 8) & 0xFF) / 255.0f;
    float b = (color & 0xFF) / 255.0f;

    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    const int segs = 28;
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; ++i) {
        float ang = (2.0f * PI * i) / segs;
        float ca = std::cos(ang);
        float sa = std::sin(ang);
        float edgeFade = 0.55f + 0.45f * std::cos(ang);

        glColor4f(r, g, b, alpha * 0.32f * edgeFade);
        glVertex3f(x + ca * topR, yTop, z + sa * topR);

        glColor4f(r, g, b, 0.0f);
        glVertex3f(x + ca * botR, yBot, z + sa * botR);
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glColor4f(r, g, b, alpha * 0.42f);
    glVertex3f(x, yBot + 0.02f, z);
    glColor4f(r, g, b, 0.0f);
    for (int i = 0; i <= segs; ++i) {
        float ang = (2.0f * PI * i) / segs;
        glVertex3f(x + std::cos(ang) * botR * 1.15f, yBot + 0.02f, z + std::sin(ang) * botR * 1.15f);
    }
    glEnd();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::renderBootTowersScene(float progress, const std::vector<PS2App>& apps) {
    (void)apps;
    setup3DProjection(54.0f);
    glEnable(GL_DEPTH_TEST);

    // Phase 1 (0.0 -> 0.72): Slow top-down drift while "Sony Computer Entertainment" is displayed
    // Phase 2 (0.72 -> 1.00): Plunge straight down through the center of the towers into the abyss
    float camZ = 0.8f - progress * 3.2f;
    if (progress > 0.72f) {
        float plunge = (progress - 0.72f) / 0.28f;
        camZ -= plunge * plunge * 14.5f;
    }

    float roll = -5.5f + progress * 8.0f;
    glRotatef(roll, 0.0f, 0.0f, 1.0f);
    glTranslatef(0.0f, 0.0f, -camZ);

    float fadeAlpha = 1.0f;
    if (progress > 0.82f) {
        fadeAlpha = std::clamp(1.0f - (progress - 0.82f) / 0.18f, 0.0f, 1.0f);
    }

    // 1. Render all PS2 Monolith Bars from directly Top-Down
    for (const auto& t : bootTowers) {
        drawMonolithTower(t.x, t.y, t.width, t.height, fadeAlpha, t.hollowTop);
    }

    // 2. Render the dark translucent tilted 3D cube floating in the central blue mist
    glPushMatrix();
    glTranslatef(-0.42f, 0.56f, -6.7f);
    glRotatef(28.0f + globalTime * 14.0f, 0.6f, 0.8f, 0.3f);
    glRotatef(-32.0f, 0.0f, 0.0f, 1.0f);
    float cs = 0.38f;
    glBegin(GL_QUADS);
    // Dark navy/black translucent faces matching the central floating cube in the PS2 boot sequence
    glColor4f(0.03f, 0.06f, 0.16f, 0.86f * fadeAlpha);
    glVertex3f(-cs, -cs,  cs); glVertex3f( cs, -cs,  cs); glVertex3f( cs,  cs,  cs); glVertex3f(-cs,  cs,  cs);
    glColor4f(0.06f, 0.12f, 0.28f, 0.86f * fadeAlpha);
    glVertex3f(-cs,  cs, -cs); glVertex3f(-cs,  cs,  cs); glVertex3f( cs,  cs,  cs); glVertex3f( cs,  cs, -cs);
    glColor4f(0.02f, 0.04f, 0.10f, 0.86f * fadeAlpha);
    glVertex3f(-cs, -cs, -cs); glVertex3f(-cs, -cs,  cs); glVertex3f(-cs,  cs,  cs); glVertex3f(-cs,  cs, -cs);
    glColor4f(0.08f, 0.16f, 0.36f, 0.86f * fadeAlpha);
    glVertex3f( cs, -cs, -cs); glVertex3f( cs,  cs, -cs); glVertex3f( cs,  cs,  cs); glVertex3f( cs, -cs,  cs);
    glEnd();
    glPopMatrix();

    // 3. Render the 4 colored pinpoint orbs (Green, Red, Magenta, Violet) with thin laser tails
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    struct LaserOrb {
        uint32_t color;
        float baseX, baseY, baseZ;
        float velX, velY;
    };
    const LaserOrb orbs[4] = {
        { 0xFF38FF48, -1.62f, -0.68f, -7.2f,  0.52f,  0.04f }, // Green (lower-left streaking right into hollow bar)
        { 0xFFFF2434,  0.18f,  0.42f, -7.8f, -0.46f,  0.12f }, // Red (center-right streaking left)
        { 0xFFFF48B8,  0.92f,  0.18f, -8.4f, -0.38f,  0.08f }, // Magenta/Pink (mid-right)
        { 0xFF6848FF, -1.24f, -1.32f, -8.6f,  0.34f,  0.16f }  // Violet/Blue (lower-left)
    };

    float drift = std::sin(globalTime * 0.9f) * 0.18f;
    for (int i = 0; i < 4; ++i) {
        float ox = orbs[i].baseX + orbs[i].velX * drift;
        float oy = orbs[i].baseY + orbs[i].velY * drift;
        float oz = orbs[i].baseZ;

        float r = ((orbs[i].color >> 16) & 0xFF) / 255.0f;
        float g = ((orbs[i].color >> 8) & 0xFF) / 255.0f;
        float b = (orbs[i].color & 0xFF) / 255.0f;

        // Thin colored laser tail streaking behind the pinpoint orb
        glLineWidth(2.0f * dpiScale);
        glBegin(GL_LINES);
        glColor4f(r, g, b, 0.85f * fadeAlpha);
        glVertex3f(ox, oy, oz);
        glColor4f(r, g, b, 0.0f);
        glVertex3f(ox - orbs[i].velX * 0.95f, oy - orbs[i].velY * 0.95f, oz - 0.2f);
        glEnd();

        // Small, crisp pinpoint orb glow
        drawGlowBillboard(ox, oy, oz, 0.16f, orbs[i].color, 0.95f * fadeAlpha, 0.38f);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::renderBackgroundGlassCubes(float time, float alphaScale) {
    setup3DProjection(40.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(orbClusterCenter.x, orbClusterCenter.y, orbClusterCenter.z - 2.6f);
    glRotatef(18.0f, 1.0f, 0.0f, 0.2f);
    glRotatef(time * 7.5f, 0.0f, 1.0f, 0.0f);

    float ringR = 2.25f;
    for (int i = 0; i < 7; ++i) {
        float ang = (2.0f * PI * i) / 7.0f;
        glPushMatrix();
        glTranslatef(std::cos(ang) * ringR, std::sin(ang * 2.0f) * 0.35f, std::sin(ang) * ringR);
        glRotatef(time * 22.0f + i * 51.0f, 1.0f, 1.0f, 0.4f);

        float s = 0.32f;
        glColor4f(0.10f, 0.24f, 0.58f, 0.12f * alphaScale);
        glBegin(GL_QUADS);
        glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s); glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);
        glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s); glVertex3f( s,  s, -s); glVertex3f( s, -s, -s);
        glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s); glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);
        glVertex3f(-s, -s, -s); glVertex3f( s, -s, -s); glVertex3f( s, -s,  s); glVertex3f(-s, -s,  s);
        glEnd();

        glLineWidth(1.1f * dpiScale);
        glColor4f(0.28f, 0.55f, 0.95f, 0.18f * alphaScale);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s); glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::renderSevenStarsOrbs(float alphaScale) {
    setup3DProjection(40.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    // Subtle deep-blue Eevee-style ambient volume glow centered on the wobbling coin plane
    Vec3 ringCenter = evaluateOrbCirclePoint(0.0f, globalTime, orbClusterCenter, 0.0f);
    drawGlowBillboard(ringCenter.x, ringCenter.y, ringCenter.z, 1.95f, 0xFF1642B8, 0.28f * alphaScale, 0.15f);

    // 1. Render soft curved motion-blur wake along the parented circular curve for each of the 7 orbs
    for (const auto& orb : sevenStars) {
        if (orb.trail.size() < 2) continue;

        // Overlapping soft additive emission glow sprites along the circular arc
        for (size_t k = 2; k < orb.trail.size(); k += 2) {
            float frac = 1.0f - static_cast<float>(k) / static_cast<float>(orb.trail.size());
            float wakeAlpha = 0.13f * frac * frac * alphaScale;
            float wakeRadius = (0.09f + 0.11f * frac);
            const Vec3& p = orb.trail[k];
            drawGlowBillboard(p.x, p.y, p.z, wakeRadius, 0xFF3A82FF, wakeAlpha, 0.22f);
        }
    }

    // 2. Render the 7 small emissive spheres ("The Dancing Dots") snapped to the spinning circular curve
    for (const auto& orb : sevenStars) {
        const Vec3& head = orb.currentPos;
        // Outer soft cyan-blue Eevee bloom halo
        drawGlowBillboard(head.x, head.y, head.z, 0.28f, orb.color, 0.85f * alphaScale, 0.36f);
        // Crisp, small, intense white emissive sphere core
        drawGlowBillboard(head.x, head.y, head.z, 0.11f, 0xFFFFFFFF, 1.0f * alphaScale, 0.62f);
    }

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::renderSystemConfigCrystal(float time, int selectedOptionIndex) {
    setup3DProjection(40.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glPushMatrix();
    glTranslatef(-1.65f, 0.05f, -5.6f);

    float ringAlpha = (selectedOptionIndex == 0) ? 0.95f : 0.45f;
    glPushMatrix();
    glRotatef(24.0f, 1.0f, 0.25f, 0.0f);
    glRotatef(-time * 10.0f, 0.0f, 0.0f, 1.0f);
    float ringR = 1.48f;
    glLineWidth(1.4f * dpiScale);
    glColor4f(0.25f, 0.58f, 1.0f, 0.40f * ringAlpha);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < 64; ++i) {
        float a = (2.0f * PI * i) / 64.0f;
        glVertex3f(std::cos(a) * ringR, std::sin(a) * ringR, 0.0f);
    }
    glEnd();

    for (int h = 0; h < 12; ++h) {
        float a = (2.0f * PI * h) / 12.0f;
        drawGlowBillboard(std::cos(a) * ringR, std::sin(a) * ringR, 0.0f, 0.13f, 0xFF80D8FF, 0.80f * ringAlpha);
    }
    glPopMatrix();

    glRotatef(time * 22.0f, 0.35f, 1.0f, 0.25f);
    float s = 1.05f;
    const Vec3 top = {0.0f, s, 0.0f};
    const Vec3 bot = {0.0f, -s, 0.0f};
    const Vec3 v[4] = {{s, 0, 0}, {0, 0, s}, {-s, 0, 0}, {0, 0, -s}};

    glBegin(GL_TRIANGLES);
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        glColor4f(0.16f, 0.44f, 0.92f, 0.24f);
        glVertex3f(top.x, top.y, top.z);
        glColor4f(0.06f, 0.20f, 0.58f, 0.18f);
        glVertex3f(v[i].x, v[i].y, v[i].z);
        glVertex3f(v[next].x, v[next].y, v[next].z);

        glColor4f(0.12f, 0.34f, 0.82f, 0.24f);
        glVertex3f(bot.x, bot.y, bot.z);
        glColor4f(0.06f, 0.18f, 0.52f, 0.18f);
        glVertex3f(v[next].x, v[next].y, v[next].z);
        glVertex3f(v[i].x, v[i].y, v[i].z);
    }
    glEnd();

    glLineWidth(1.8f * dpiScale);
    glColor4f(0.50f, 0.80f, 1.0f, 0.58f);
    glBegin(GL_LINES);
    for (int i = 0; i < 4; ++i) {
        int next = (i + 1) % 4;
        glVertex3f(top.x, top.y, top.z); glVertex3f(v[i].x, v[i].y, v[i].z);
        glVertex3f(bot.x, bot.y, bot.z); glVertex3f(v[i].x, v[i].y, v[i].z);
        glVertex3f(v[i].x, v[i].y, v[i].z); glVertex3f(v[next].x, v[next].y, v[next].z);
    }
    glEnd();

    glPopMatrix();
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_TRUE);
}

void Renderer::drawMemoryCard3D(float x, float y, float z, float scale, int slotNum, bool selected) {
    glPushMatrix();
    float bob = selected ? std::sin(globalTime * 3.5f) * 0.07f : 0.0f;
    glTranslatef(x, y + bob, z);
    glScalef(scale, scale, scale);
    glRotatef(22.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(selected ? std::sin(globalTime * 2.2f) * 18.0f : (slotNum == 1 ? -12.0f : 12.0f), 0.0f, 1.0f, 0.0f);

    float w = 0.46f;
    float h = 0.58f;
    float d = 0.065f;

    float boost = selected ? 1.35f : 0.78f;
    float br = (slotNum == 1 ? 0.14f : 0.10f) * boost;
    float bg = (slotNum == 1 ? 0.16f : 0.22f) * boost;
    float bb = (slotNum == 1 ? 0.22f : 0.48f) * boost;

    glBegin(GL_QUADS);
    glColor4f(br, bg, bb, 0.96f);
    glVertex3f(-w, -h,  d); glVertex3f( w, -h,  d); glVertex3f( w,  h,  d); glVertex3f(-w,  h,  d);
    glColor4f(br * 0.7f, bg * 0.7f, bb * 0.7f, 0.96f);
    glVertex3f(-w, -h, -d); glVertex3f(-w,  h, -d); glVertex3f( w,  h, -d); glVertex3f( w, -h, -d);
    glColor4f(br * 0.85f, bg * 0.85f, bb * 0.85f, 0.96f);
    glVertex3f(-w, -h, -d); glVertex3f(-w, -h,  d); glVertex3f(-w,  h,  d); glVertex3f(-w,  h, -d);
    glVertex3f( w, -h, -d); glVertex3f( w,  h, -d); glVertex3f( w,  h,  d); glVertex3f( w, -h,  d);
    glVertex3f(-w,  h, -d); glVertex3f(-w,  h,  d); glVertex3f( w,  h,  d); glVertex3f( w,  h, -d);

    glColor4f(0.12f * boost, 0.38f * boost, 0.85f * boost, 0.98f);
    glVertex3f(-w * 0.78f, -0.05f, d + 0.004f);
    glVertex3f( w * 0.78f, -0.05f, d + 0.004f);
    glVertex3f( w * 0.78f,  h * 0.76f, d + 0.004f);
    glVertex3f(-w * 0.78f,  h * 0.76f, d + 0.004f);
    glEnd();

    glBegin(GL_TRIANGLES);
    glColor4f(0.95f * boost, 0.78f * boost, 0.25f * boost, 0.95f);
    glVertex3f(0.0f, -h * 0.82f, d + 0.005f);
    glVertex3f(0.10f, -h * 0.56f, d + 0.005f);
    glVertex3f(-0.10f, -h * 0.56f, d + 0.005f);
    glEnd();

    glLineWidth((selected ? 2.2f : 1.2f) * dpiScale);
    glColor4f(selected ? 0.98f : 0.42f, selected ? 0.84f : 0.56f, selected ? 0.38f : 0.82f, 0.88f);
    glBegin(GL_LINE_LOOP);
    glVertex3f(-w, -h, d + 0.005f);
    glVertex3f( w, -h, d + 0.005f);
    glVertex3f( w,  h, d + 0.005f);
    glVertex3f(-w,  h, d + 0.005f);
    glEnd();

    glPopMatrix();
}

void Renderer::drawPS2Disc3D(float x, float y, float z, float radius, float spinDeg, uint32_t tintColor, bool selected) {
    glPushMatrix();
    float bob = selected ? std::sin(globalTime * 3.5f) * 0.07f : 0.0f;
    glTranslatef(x, y + bob, z);
    glRotatef(32.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(spinDeg, 0.0f, 1.0f, 0.0f);

    float rOut = radius;
    float rIn = radius * 0.23f;
    int segs = 48;

    float tr = ((tintColor >> 16) & 0xFF) / 255.0f;
    float tg = ((tintColor >> 8) & 0xFF) / 255.0f;
    float tb = (tintColor & 0xFF) / 255.0f;
    float boost = selected ? 1.15f : 0.82f;

    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= segs; ++i) {
        float ang = (2.0f * PI * i) / segs;
        float ca = std::cos(ang);
        float sa = std::sin(ang);
        float sheen = 0.5f + 0.5f * std::sin(ang * 2.0f + spinDeg * 0.04f);

        float cr = std::min(1.0f, (0.78f * sheen + tr * 0.35f) * boost);
        float cg = std::min(1.0f, (0.68f * sheen + tg * 0.35f) * boost);
        float cb = std::min(1.0f, (0.38f + sheen * 0.50f + tb * 0.25f) * boost);

        glColor4f(cr, cg, cb, 0.95f);
        glVertex3f(ca * rOut, 0.0f, sa * rOut);
        glColor4f(0.92f * boost, 0.96f * boost, 1.0f, 0.95f);
        glVertex3f(ca * rIn, 0.0f, sa * rIn);
    }
    glEnd();

    glLineWidth((selected ? 2.2f : 1.3f) * dpiScale);
    glColor4f(selected ? 1.0f : 0.70f, selected ? 0.90f : 0.80f, selected ? 0.55f : 0.95f, 0.90f);
    glBegin(GL_LINE_LOOP);
    for (int i = 0; i < segs; ++i) {
        float ang = (2.0f * PI * i) / segs;
        glVertex3f(std::cos(ang) * rOut, 0.005f, std::sin(ang) * rOut);
    }
    glEnd();

    glPopMatrix();
}

void Renderer::drawFolderMonolith3D(float x, float y, float z, bool selected) {
    glPushMatrix();
    float bob = selected ? std::sin(globalTime * 3.5f) * 0.06f : 0.0f;
    glTranslatef(x, y + bob, z);
    glRotatef(-12.0f + std::sin(globalTime * 1.2f) * 6.0f, 0.0f, 0.0f, 1.0f);

    float boost = selected ? 1.18f : 0.82f;
    const float offsets[4][3] = {
        {-0.22f,  0.18f,  0.15f},
        { 0.22f,  0.16f, -0.05f},
        {-0.18f, -0.22f, -0.12f},
        { 0.20f, -0.18f,  0.22f}
    };

    for (int i = 0; i < 4; ++i) {
        float ox = offsets[i][0];
        float oy = offsets[i][1];
        float zTop = offsets[i][2];
        float zBase = -1.4f;
        float w = 0.14f;

        glBegin(GL_QUADS);
        // Side shafts receding along -Z into the screen
        glColor4f(0.45f * boost, 0.54f * boost, 0.72f * boost, 0.92f);
        glVertex3f(ox - w, oy + w, zTop); glVertex3f(ox + w, oy + w, zTop);
        glColor4f(0.03f, 0.07f, 0.18f, 0.92f);
        glVertex3f(ox + w, oy + w, zBase); glVertex3f(ox - w, oy + w, zBase);

        glColor4f(0.38f * boost, 0.46f * boost, 0.64f * boost, 0.92f);
        glVertex3f(ox + w, oy - w, zTop); glVertex3f(ox - w, oy - w, zTop);
        glColor4f(0.03f, 0.07f, 0.18f, 0.92f);
        glVertex3f(ox - w, oy - w, zBase); glVertex3f(ox + w, oy - w, zBase);

        glColor4f(0.42f * boost, 0.50f * boost, 0.68f * boost, 0.92f);
        glVertex3f(ox - w, oy - w, zTop); glVertex3f(ox - w, oy + w, zTop);
        glColor4f(0.03f, 0.07f, 0.18f, 0.92f);
        glVertex3f(ox - w, oy + w, zBase); glVertex3f(ox - w, oy - w, zBase);

        glColor4f(0.48f * boost, 0.58f * boost, 0.76f * boost, 0.92f);
        glVertex3f(ox + w, oy + w, zTop); glVertex3f(ox + w, oy - w, zTop);
        glColor4f(0.03f, 0.07f, 0.18f, 0.92f);
        glVertex3f(ox + w, oy - w, zBase); glVertex3f(ox + w, oy + w, zBase);

        // Top-down square stone cap facing the viewer
        glColor4f(0.72f * boost, 0.78f * boost, 0.88f * boost, 0.96f);
        glVertex3f(ox - w, oy - w, zTop);
        glVertex3f(ox + w, oy - w, zTop);
        glVertex3f(ox + w, oy + w, zTop);
        glVertex3f(ox - w, oy + w, zTop);
        glEnd();
    }

    glPopMatrix();
}

void Renderer::renderBrowserDevices3D(int selectedDeviceIdx, const std::vector<PS2App>& apps) {
    setup3DProjection(40.0f);
    glEnable(GL_DEPTH_TEST);

    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight > 0 ? windowHeight : 1);
    float spread = std::min(1.0f, aspect / 1.68f);

    const float xs[4] = {-2.55f * spread, -0.85f * spread, 0.85f * spread, 2.55f * spread};
    const float z = -6.2f;
    const float y = 0.15f;

    drawMemoryCard3D(xs[0], y, z, 1.15f * spread, 1, selectedDeviceIdx == 0);
    drawMemoryCard3D(xs[1], y, z, 1.15f * spread, 2, selectedDeviceIdx == 1);

    uint32_t discCol = !apps.empty() ? apps[0].discColor : 0xFFFFD54F;
    drawPS2Disc3D(xs[2], y, z, 0.62f * spread, discSpinAngle, discCol, selectedDeviceIdx == 2);
    drawFolderMonolith3D(xs[3], y, z, selectedDeviceIdx == 3);

    if (selectedDeviceIdx >= 0 && selectedDeviceIdx < 4) {
        drawVolumetricSpotlight(xs[selectedDeviceIdx], 2.4f, -0.95f, z, 0.25f, 0.85f, 0xFF80D8FF, 0.85f);
    }
}

void Renderer::drawSaveIcon3D(
    float x, float y, float z, size_t index,
    const PS2App& app, bool selected, float time
) {
    glPushMatrix();
    float bob = selected ? (0.10f + std::sin(time * 4.2f) * 0.06f) : 0.0f;
    glTranslatef(x, y + bob, z);

    float scale = selected ? 1.32f : 0.92f;
    glScalef(scale, scale, scale);
    glRotatef(22.0f, 1.0f, 0.0f, 0.0f);
    glRotatef(time * (selected ? 72.0f : 24.0f) + index * 47.0f, 0.0f, 1.0f, 0.0f);

    float r = ((app.discColor >> 16) & 0xFF) / 255.0f;
    float g = ((app.discColor >> 8) & 0xFF) / 255.0f;
    float b = (app.discColor & 0xFF) / 255.0f;
    float boost = selected ? 1.25f : 0.85f;
    r = std::min(1.0f, r * boost);
    g = std::min(1.0f, g * boost);
    b = std::min(1.0f, b * boost);

    int shape = static_cast<int>(index % 4);
    float s = 0.28f;

    if (shape == 0) {
        glBegin(GL_TRIANGLES);
        const Vec3 top = {0, s * 1.35f, 0};
        const Vec3 bot = {0, -s * 1.35f, 0};
        const Vec3 v[4] = {{s, 0, 0}, {0, 0, s}, {-s, 0, 0}, {0, 0, -s}};
        for (int i = 0; i < 4; ++i) {
            int n = (i + 1) % 4;
            glColor4f(r, g, b, 0.95f);
            glVertex3f(top.x, top.y, top.z); glVertex3f(v[i].x, v[i].y, v[i].z); glVertex3f(v[n].x, v[n].y, v[n].z);
            glColor4f(r * 0.65f, g * 0.65f, b * 0.65f, 0.95f);
            glVertex3f(bot.x, bot.y, bot.z); glVertex3f(v[n].x, v[n].y, v[n].z); glVertex3f(v[i].x, v[i].y, v[i].z);
        }
        glEnd();
    } else if (shape == 1) {
        glBegin(GL_QUADS);
        glColor4f(r, g, b, 0.94f);
        glVertex3f(-s, -s,  s); glVertex3f( s, -s,  s); glVertex3f( s,  s,  s); glVertex3f(-s,  s,  s);
        glColor4f(r * 0.7f, g * 0.7f, b * 0.7f, 0.94f);
        glVertex3f(-s, -s, -s); glVertex3f(-s,  s, -s); glVertex3f( s,  s, -s); glVertex3f( s, -s, -s);
        glColor4f(r * 0.85f, g * 0.85f, b * 0.85f, 0.94f);
        glVertex3f(-s,  s, -s); glVertex3f(-s,  s,  s); glVertex3f( s,  s,  s); glVertex3f( s,  s, -s);
        glVertex3f( s, -s, -s); glVertex3f( s,  s, -s); glVertex3f( s,  s,  s); glVertex3f( s, -s,  s);
        glVertex3f(-s, -s, -s); glVertex3f(-s, -s,  s); glVertex3f(-s,  s,  s); glVertex3f(-s,  s, -s);
        glEnd();
    } else if (shape == 2) {
        glLineWidth(2.4f * dpiScale);
        glColor4f(r, g, b, 0.95f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 28; ++i) {
            float a = (2.0f * PI * i) / 28.0f;
            glVertex3f(std::cos(a) * s * 1.25f, std::sin(a) * s * 1.25f, 0.0f);
        }
        glEnd();
        glColor4f(0.95f, 0.90f, 0.55f, 0.95f);
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 28; ++i) {
            float a = (2.0f * PI * i) / 28.0f;
            glVertex3f(0.0f, std::cos(a) * s * 1.0f, std::sin(a) * s * 1.0f);
        }
        glEnd();
        glPopMatrix();
        glDepthMask(GL_FALSE);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);
        drawGlowBillboard(x, y + bob, z, 0.28f * scale, app.discColor, 0.95f);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_TRUE);
        return;
    } else {
        glPopMatrix();
        drawPS2Disc3D(x, y + bob, z, 0.34f * scale, time * (selected ? 140.0f : 35.0f) + index * 25.0f, app.discColor, selected);
        return;
    }

    if (selected) {
        glLineWidth(1.8f * dpiScale);
        glColor4f(1.0f, 0.92f, 0.55f, 0.90f);
        glBegin(GL_LINE_LOOP);
        glVertex3f(-s * 1.08f, -s * 1.08f, s * 1.08f);
        glVertex3f( s * 1.08f, -s * 1.08f, s * 1.08f);
        glVertex3f( s * 1.08f,  s * 1.08f, s * 1.08f);
        glVertex3f(-s * 1.08f,  s * 1.08f, s * 1.08f);
        glEnd();
    }

    glPopMatrix();
}

void Renderer::renderBrowserGrid3D(
    const std::vector<PS2App>& apps,
    size_t selectedIdx,
    bool inActionMenu,
    int activeSlot
) {
    setup3DProjection(40.0f);
    glEnable(GL_DEPTH_TEST);

    float aspect = static_cast<float>(windowWidth) / static_cast<float>(windowHeight > 0 ? windowHeight : 1);
    float cardX = -1.78f * aspect;
    drawMemoryCard3D(cardX, 1.72f, -5.8f, 0.45f, activeSlot + 1, true);

    if (apps.empty()) return;

    if (inActionMenu && selectedIdx < apps.size()) {
        drawSaveIcon3D(-1.35f, 0.05f, -5.2f, selectedIdx, apps[selectedIdx], true, globalTime);
        drawVolumetricSpotlight(-1.35f, 2.2f, -1.0f, -5.2f, 0.22f, 0.85f, apps[selectedIdx].discColor, 0.90f);
        return;
    }

    int selRow = static_cast<int>(selectedIdx / 4);
    int topRow = std::max(0, selRow - 1);
    targetGridScrollY = topRow * 1.18f;

    float spread = std::min(1.0f, aspect / 1.65f);
    const float colX[4] = {-2.05f * spread, -0.68f * spread, 0.68f * spread, 2.05f * spread};
    const float baseZ = -6.2f;

    Vec3 selectedPos = {0.0f, 0.0f, baseZ};

    for (size_t i = 0; i < apps.size(); ++i) {
        int r = static_cast<int>(i / 4);
        int c = static_cast<int>(i % 4);

        float worldY = 0.85f - r * 1.18f + gridScrollY;
        if (worldY > 2.1f || worldY < -2.1f) continue;

        bool isSel = (i == selectedIdx);
        float zOffset = isSel ? 0.35f : 0.0f;
        drawSaveIcon3D(colX[c], worldY, baseZ + zOffset, i, apps[i], isSel, globalTime);

        if (isSel) {
            selectedPos = {colX[c], worldY, baseZ + zOffset};
        }
    }

    drawVolumetricSpotlight(
        selectedPos.x,
        selectedPos.y + 1.85f,
        selectedPos.y - 0.55f,
        selectedPos.z,
        0.18f,
        0.58f,
        0xFF90E0FF,
        0.85f
    );
}

void Renderer::render(
    float deltaTime,
    ScreenState state,
    MenuTarget target,
    float bootProgress,
    const AppScanner& appScanner,
    int selectedDeviceIndex,
    size_t selectedAppIndex,
    int selectedActionIndex,
    size_t selectedDirIndex,
    const GamepadManager& gamepad,
    const SoundSettings& soundSettings,
    int activeMemorySlot,
    int selectedOptionIndex
) {
    updateSimulations(deltaTime, state, target);

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    renderPS2BackgroundVoid(state, bootProgress);

    const auto& discs = appScanner.getDiscs();

    if (state == ScreenState::BOOT) {
        renderBootTowersScene(bootProgress, discs);
    } else if (state == ScreenState::MAIN_MENU) {
        renderBackgroundGlassCubes(globalTime, 1.0f);
        renderSevenStarsOrbs(1.0f);
    } else if (state == ScreenState::BROWSER_DEVICES) {
        renderSevenStarsOrbs(0.35f);
        renderBrowserDevices3D(selectedDeviceIndex, discs);
    } else if (state == ScreenState::BROWSER_GRID || state == ScreenState::BROWSER_ACTION) {
        renderSevenStarsOrbs(0.22f);
        renderBrowserGrid3D(discs, selectedAppIndex, state == ScreenState::BROWSER_ACTION, activeMemorySlot);
    } else if (state == ScreenState::SYSTEM_CONFIG || state == ScreenState::VERSION_INFO) {
        renderSevenStarsOrbs(0.75f);
        renderSystemConfigCrystal(globalTime, selectedOptionIndex);
    } else if (state == ScreenState::DIR_MAPPING_MODAL) {
        renderSevenStarsOrbs(0.40f);
    }

    render2DOverlay(
        state, target, bootProgress, appScanner,
        selectedDeviceIndex, selectedAppIndex, selectedActionIndex, selectedDirIndex,
        gamepad, soundSettings, activeMemorySlot, selectedOptionIndex
    );
}

void Renderer::drawPlayStationButton(int type, float cx, float cy, float r) {
    TextRenderer::drawRoundedRect(cx - r, cy - r, r * 2.0f, r * 2.0f, r, 0xEE222B42, 0xEE0C101C);
    TextRenderer::drawRoundedRectOutline(cx - r, cy - r, r * 2.0f, r * 2.0f, r, 1.4f, 0x777898C8);

    glLineWidth(2.2f * dpiScale);
    if (type == 0) {
        glColor4f(0.48f, 0.72f, 1.0f, 1.0f);
        float d = r * 0.46f;
        glBegin(GL_LINES);
        glVertex2f(cx - d, cy - d); glVertex2f(cx + d, cy + d);
        glVertex2f(cx + d, cy - d); glVertex2f(cx - d, cy + d);
        glEnd();
    } else if (type == 1) {
        glColor4f(0.98f, 0.36f, 0.32f, 1.0f);
        float cr = r * 0.50f;
        glBegin(GL_LINE_LOOP);
        for (int i = 0; i < 24; ++i) {
            float a = (2.0f * PI * i) / 24.0f;
            glVertex2f(cx + std::cos(a) * cr, cy + std::sin(a) * cr);
        }
        glEnd();
    } else if (type == 2) {
        glColor4f(0.92f, 0.52f, 0.80f, 1.0f);
        float d = r * 0.45f;
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx - d, cy - d);
        glVertex2f(cx + d, cy - d);
        glVertex2f(cx + d, cy + d);
        glVertex2f(cx - d, cy + d);
        glEnd();
    } else if (type == 3) {
        glColor4f(0.25f, 0.88f, 0.62f, 1.0f);
        float d = r * 0.50f;
        glBegin(GL_LINE_LOOP);
        glVertex2f(cx, cy - d);
        glVertex2f(cx + d, cy + d * 0.8f);
        glVertex2f(cx - d, cy + d * 0.8f);
        glEnd();
    }
    glLineWidth(1.0f);
}

void Renderer::render2DOverlay(
    ScreenState state,
    MenuTarget target,
    float bootProgress,
    const AppScanner& appScanner,
    int selectedDeviceIndex,
    size_t selectedAppIndex,
    int selectedActionIndex,
    size_t selectedDirIndex,
    const GamepadManager& gamepad,
    const SoundSettings& soundSettings,
    int activeMemorySlot,
    int selectedOptionIndex
) {
    glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    auto& tr = TextRenderer::instance();

    if (state == ScreenState::BOOT) {
        if (bootProgress >= 0.03f && bootProgress <= 0.78f) {
            float fadeIn  = std::clamp((bootProgress - 0.03f) / 0.14f, 0.0f, 1.0f);
            float fadeOut = std::clamp((0.78f - bootProgress) / 0.14f, 0.0f, 1.0f);
            float alpha = std::min(fadeIn, fadeOut);
            uint32_t alphaMask = static_cast<uint32_t>(alpha * 232.0f) << 24;

            tr.drawText(
                "Sony Computer Entertainment",
                windowWidth * 0.49f,
                windowHeight * 0.475f,
                38.0f,
                alphaMask | 0x00BEC8D8,
                TextAlign::CENTER,
                true,
                true
            );
        }
        return;
    }

    switch (state) {
        case ScreenState::MAIN_MENU:
            renderMainMenuOverlay(target);
            break;
        case ScreenState::BROWSER_DEVICES:
            renderBrowserDevicesOverlay(selectedDeviceIndex, appScanner);
            break;
        case ScreenState::BROWSER_GRID:
        case ScreenState::BROWSER_ACTION:
            renderBrowserGridOverlay(
                appScanner, selectedAppIndex, activeMemorySlot,
                state == ScreenState::BROWSER_ACTION, selectedActionIndex
            );
            break;
        case ScreenState::SYSTEM_CONFIG:
            renderSystemConfigOverlay(soundSettings, selectedOptionIndex);
            break;
        case ScreenState::VERSION_INFO:
            renderVersionOverlay(appScanner, gamepad);
            break;
        case ScreenState::DIR_MAPPING_MODAL:
            renderDirMapperOverlay(appScanner.getMappedDirectories(), selectedDirIndex, appScanner.getDiscs().size());
            break;
        default:
            break;
    }

    if (gamepad.hasActiveToast()) {
        renderGamepadToast(gamepad.getActiveToast());
    }

    renderPS2PromptBar(state, gamepad.isConnected());
}

void Renderer::renderMainMenuOverlay(MenuTarget target) {
    auto& tr = TextRenderer::instance();

    const char* items[2] = {
        "Browser",
        "System Configuration"
    };

    float menuX = windowWidth * 0.57f;
    float startY = windowHeight * 0.43f;
    float spacing = 64.0f;
    int activeIdx = (target == MenuTarget::BROWSER) ? 0 : 1;

    for (int i = 0; i < 2; ++i) {
        bool isSel = (i == activeIdx);
        float y = startY + i * spacing;

        uint32_t col = isSel ? 0xFFF4C852 : 0xFF4E6E9E;
        tr.drawText(items[i], menuX, y, 31.0f, col, TextAlign::LEFT, true, isSel);
    }
}

void Renderer::renderBrowserDevicesOverlay(int selectedDeviceIdx, const AppScanner& appScanner) {
    auto& tr = TextRenderer::instance();
    const auto& apps = appScanner.getDiscs();

    tr.drawText("Browser", 72.0f, 48.0f, 28.0f, 0xFF6B8BB8, TextAlign::LEFT, true, false);

    std::string devTitle;
    std::string devStatus;
    if (selectedDeviceIdx == 0) {
        devTitle = "Memory Card (PS2)  /  Slot 1";
        devStatus = std::to_string(apps.size()) + " Titles Mounted  (8MB MagicGate)";
    } else if (selectedDeviceIdx == 1) {
        devTitle = "Memory Card (PS2)  /  Slot 2";
        devStatus = std::to_string(apps.size()) + " Titles Mounted  (8MB MagicGate)";
    } else if (selectedDeviceIdx == 2) {
        devTitle = "PlayStation 2 Format Disc";
        devStatus = !apps.empty() ? apps[0].title : "No Disc Inserted";
    } else {
        devTitle = "Local Directory Mounts";
        devStatus = std::to_string(appScanner.getMappedDirectories().size()) + " macOS Folders Mapped";
    }

    tr.drawText(devTitle, windowWidth - 78.0f, 52.0f, 22.0f, 0xFFF4C852, TextAlign::RIGHT, true, false);
    tr.drawText(devStatus, windowWidth * 0.5f, windowHeight * 0.74f, 24.0f, 0xFFD8E8FF, TextAlign::CENTER, true, true);
}

void Renderer::renderBrowserGridOverlay(
    const AppScanner& appScanner,
    size_t selectedIdx,
    int activeSlot,
    bool inActionMenu,
    int selectedActionIdx
) {
    auto& tr = TextRenderer::instance();
    const auto& apps = appScanner.getDiscs();

    std::string slotLabel = "Memory Card (PS2)  /  Slot " + std::to_string(activeSlot + 1);
    tr.drawText(slotLabel, 148.0f, 52.0f, 22.0f, 0xFF7B9CC8, TextAlign::LEFT, true, false);

    int usedKB = static_cast<int>(apps.size()) * 142;
    int freeKB = std::max(128, 7998 - usedKB);
    std::string freeStr = std::to_string(freeKB) + " KB Free";
    tr.drawText(freeStr, windowWidth - 78.0f, 52.0f, 22.0f, 0xFF7B9CC8, TextAlign::RIGHT, true, false);

    if (apps.empty() || selectedIdx >= apps.size()) {
        tr.drawText("No Data", windowWidth * 0.5f, windowHeight * 0.5f, 28.0f, 0xFF7B9CC8, TextAlign::CENTER, true, false);
        return;
    }

    const auto& cur = apps[selectedIdx];

    if (inActionMenu) {
        tr.drawText(cur.title, windowWidth * 0.30f, windowHeight * 0.68f, 26.0f, 0xFFF4C852, TextAlign::CENTER, true, true);
        std::string kbStr = std::to_string(cur.saveBlocks * 18) + " KB   •   " + cur.category;
        tr.drawText(kbStr, windowWidth * 0.30f, windowHeight * 0.74f, 17.0f, 0xFF7B9CC8, TextAlign::CENTER, false, false);

        const char* actions[3] = {
            "Launch Application",
            "Reveal in Finder",
            "Cancel"
        };
        float actX = windowWidth * 0.58f;
        float actY = windowHeight * 0.40f;
        for (int i = 0; i < 3; ++i) {
            bool isSel = (i == selectedActionIdx);
            uint32_t col = isSel ? 0xFFF4C852 : 0xFF5274A6;
            tr.drawText(actions[i], actX, actY + i * 56.0f, 28.0f, col, TextAlign::LEFT, true, isSel);
        }
        return;
    }

    tr.drawText(cur.title, 96.0f, windowHeight - 122.0f, 27.0f, 0xFFF4C852, TextAlign::LEFT, true, true);

    std::string meta = std::to_string(cur.saveBlocks * 18) + " KB   •   " + cur.path;
    if (meta.length() > 68) {
        meta = meta.substr(0, 65) + "...";
    }
    tr.drawText(meta, 96.0f, windowHeight - 84.0f, 15.5f, 0xFF6B8BB8, TextAlign::LEFT, false, false);
}

void Renderer::renderSystemConfigOverlay(const SoundSettings& settings, int selectedIdx) {
    auto& tr = TextRenderer::instance();

    tr.drawText("System Configuration", 72.0f, 48.0f, 28.0f, 0xFF6B8BB8, TextAlign::LEFT, true, false);

    time_t rawtime;
    time(&rawtime);
    struct tm* ti = localtime(&rawtime);
    char timeStr[64];
    strftime(timeStr, sizeof(timeStr), "%Y/%m/%d   %H:%M:%S", ti);

    const char* screenSizes[3] = {"16:9", "4:3", "Full"};
    const char* soundModes[3] = {"Stereo", "Mono", "Digital Surround"};

    struct PS2ConfigItem {
        const char* label;
        std::string value;
    };

    PS2ConfigItem items[6] = {
        {"Clock Adjustment", timeStr},
        {"Screen Size", screenSizes[settings.screenSize % 3]},
        {"Digital Out (Optical)", settings.opticalOut ? "On" : "Off"},
        {"Audio Output", soundModes[settings.mode % 3]},
        {"Ambient Soundscape", settings.ambientMusic ? "Ocean Waves (On)" : "Muted (Off)"},
        {"Speaker Channel Test", "Press  X  to Chime L/R"}
    };

    float menuX = std::min(windowWidth * 0.46f, windowWidth - 540.0f);
    float startY = 148.0f;
    float spacing = 64.0f;

    for (int i = 0; i < 6; ++i) {
        bool isSel = (i == selectedIdx);
        float y = startY + i * spacing;

        uint32_t labelCol = isSel ? 0xFFF4C852 : 0xFF5274A6;
        uint32_t valCol   = isSel ? 0xFFD8F0FF : 0xFF3E5A82;

        tr.drawText(items[i].label, menuX, y, 25.0f, labelCol, TextAlign::LEFT, true, isSel);
        tr.drawText(items[i].value, windowWidth - 84.0f, y + 2.0f, 22.0f, valCol, TextAlign::RIGHT, false, false);
    }
}

void Renderer::renderVersionOverlay(const AppScanner& appScanner, const GamepadManager& gamepad) {
    auto& tr = TextRenderer::instance();

    tr.drawText("Version Information", 72.0f, 48.0f, 28.0f, 0xFF6B8BB8, TextAlign::LEFT, true, false);

    struct VerItem {
        const char* label;
        std::string val;
    };
    VerItem lines[] = {
        {"Console", "SCPH-39001  (C++20 macOS Native)"},
        {"Browser", "2.00"},
        {"CD Player", "2.00"},
        {"PlayStation Driver", "1.10"},
        {"DVD Player", "2.12U"},
        {"Controller Port 1", gamepad.isConnected() ? gamepad.getControllerName() : "Standard DualShock / Keyboard"},
        {"Mounted Titles", std::to_string(appScanner.getDiscs().size()) + " Local Applications"}
    };

    float xLeft = std::min(windowWidth * 0.42f, windowWidth - 580.0f);
    float xRight = windowWidth - 84.0f;
    float y = 148.0f;

    for (const auto& l : lines) {
        tr.drawText(l.label, xLeft, y, 22.0f, 0xFF6B8BB8, TextAlign::LEFT, true, false);
        tr.drawText(l.val, xRight, y, 22.0f, 0xFFF4C852, TextAlign::RIGHT, false, false);
        y += 54.0f;
    }
}

void Renderer::renderDirMapperOverlay(const std::vector<std::string>& dirs, size_t selectedDirIdx, size_t totalApps) {
    auto& tr = TextRenderer::instance();

    tr.drawText("Local Directory Mounts", 72.0f, 48.0f, 28.0f, 0xFF6B8BB8, TextAlign::LEFT, true, false);
    tr.drawText(
        std::to_string(totalApps) + " Applications Indexed",
        windowWidth - 78.0f, 52.0f, 22.0f, 0xFFF4C852, TextAlign::RIGHT, true, false
    );

    float x = 110.0f;
    float y = 155.0f;

    for (size_t i = 0; i < dirs.size() && i < 7; ++i) {
        bool isSel = (i == selectedDirIdx);
        uint32_t col = isSel ? 0xFFF4C852 : 0xFF5678A8;
        std::string label = "Mount " + std::to_string(i + 1) + " :    " + dirs[i];
        tr.drawText(label, x, y + i * 56.0f, 24.0f, col, TextAlign::LEFT, true, isSel);
    }

    tr.drawText(
        "Press  Triangle (V)  to Add Folder via macOS Finder     •     Press  Square (C)  to Unmount",
        110.0f, windowHeight - 115.0f, 16.0f, 0xFF6B8BB8, TextAlign::LEFT, false, false
    );
}

void Renderer::renderGamepadToast(const GamepadToast& toast) {
    auto& tr = TextRenderer::instance();
    float w = 400.0f;
    float h = 68.0f;
    float x = windowWidth - w - 36.0f;
    float y = 26.0f;

    TextRenderer::drawGlowingPill(x, y, w, h, 0xDD081432, 0xFF5C9DFF, 0.5f);
    drawPlayStationButton(0, x + 36.0f, y + 34.0f, 15.0f);

    tr.drawText(toast.title, x + 66.0f, y + 12.0f, 14.5f, 0xFFF4C852, TextAlign::LEFT, true, false);
    tr.drawText(toast.message, x + 66.0f, y + 36.0f, 13.5f, 0xFFD8E8FF, TextAlign::LEFT, false, false);
}

void Renderer::renderPS2PromptBar(ScreenState state, bool gamepadConnected) {
    (void)gamepadConnected;
    auto& tr = TextRenderer::instance();

    float y = windowHeight - 48.0f;
    float x = std::max(windowWidth * 0.48f, windowWidth - 450.0f);

    drawPlayStationButton(0, x, y, 12.5f);
    tr.drawText("Enter", x + 22.0f, y - 11.0f, 19.0f, 0xFF8AB4F8, TextAlign::LEFT, true, false);
    x += 130.0f;

    if (state != ScreenState::MAIN_MENU) {
        drawPlayStationButton(1, x, y, 12.5f);
        tr.drawText("Back", x + 22.0f, y - 11.0f, 19.0f, 0xFF8AB4F8, TextAlign::LEFT, true, false);
        x += 125.0f;
    }

    drawPlayStationButton(3, x, y, 12.5f);
    std::string triText = "Version";
    if (state == ScreenState::BROWSER_DEVICES || state == ScreenState::BROWSER_GRID) {
        triText = "Mount Folder";
    } else if (state == ScreenState::DIR_MAPPING_MODAL) {
        triText = "Add Folder";
    }
    tr.drawText(triText, x + 22.0f, y - 11.0f, 19.0f, 0xFF8AB4F8, TextAlign::LEFT, true, false);
}
