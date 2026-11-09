#pragma once

#include <OpenGL/gl.h>
#include <vector>
#include <deque>
#include <string>
#include "Types.hpp"
#include "GamepadManager.hpp"
#include "AppScanner.hpp"

struct Vec3 {
    float x, y, z;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    void init(int logicalW, int logicalH, int drawableW, int drawableH);
    void resize(int logicalW, int logicalH, int drawableW, int drawableH);

    void render(
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
    );

    void triggerDiscLaunchSpin();

    int hitTestMainMenu(int mouseX, int mouseY) const;
    int hitTestBrowserDevices(int mouseX, int mouseY) const;
    int hitTestBrowserGrid(int mouseX, int mouseY, size_t totalApps, size_t currentSelected) const;
    int hitTestSystemConfig(int mouseX, int mouseY) const;

private:
    void updateSimulations(float dt, ScreenState state, MenuTarget target);

    // Evaluate the 3D "Coin on a Table" precessing circular curve for a given angle theta at time t
    Vec3 evaluateOrbCirclePoint(float theta, float t, const Vec3& center, float radiusScale) const;

    void setup3DProjection(float fovDegrees);
    void renderPS2BackgroundVoid(ScreenState state, float bootProgress);

    // 1:1 PS2 3D Scenes
    void renderBootTowersScene(float progress, const std::vector<PS2App>& apps);
    void renderBackgroundGlassCubes(float time, float alphaScale);
    void renderSevenStarsOrbs(float alphaScale);
    void renderSystemConfigCrystal(float time, int selectedOptionIndex);
    void renderBrowserDevices3D(int selectedDeviceIdx, const std::vector<PS2App>& apps);
    void renderBrowserGrid3D(const std::vector<PS2App>& apps, size_t selectedIdx, bool inActionMenu, int activeSlot);

    // 3D Object Primitives
    void drawMonolithTower(float x, float y, float width, float height, float alpha, bool hollowTop = false);
    void drawGlowBillboard(float x, float y, float z, float radius, uint32_t color, float alpha, float coreRatio = 0.28f);
    void drawVolumetricSpotlight(float x, float yTop, float yBot, float z, float topR, float botR, uint32_t color, float alpha);
    void drawPS2Disc3D(float x, float y, float z, float radius, float spinDeg, uint32_t tintColor, bool selected);
    void drawMemoryCard3D(float x, float y, float z, float scale, int slotNum, bool selected);
    void drawFolderMonolith3D(float x, float y, float z, bool selected);
    void drawSaveIcon3D(float x, float y, float z, size_t index, const PS2App& app, bool selected, float time);

    // 2D Frameless PS2 Overlays
    void render2DOverlay(
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
    );

    void renderMainMenuOverlay(MenuTarget target);
    void renderBrowserDevicesOverlay(int selectedDeviceIdx, const AppScanner& appScanner);
    void renderBrowserGridOverlay(
        const AppScanner& appScanner,
        size_t selectedIdx,
        int activeSlot,
        bool inActionMenu,
        int selectedActionIdx
    );
    void renderSystemConfigOverlay(const SoundSettings& settings, int selectedIdx);
    void renderVersionOverlay(const AppScanner& appScanner, const GamepadManager& gamepad);
    void renderDirMapperOverlay(const std::vector<std::string>& dirs, size_t selectedDirIdx, size_t totalApps);
    void renderGamepadToast(const GamepadToast& toast);
    void renderPS2PromptBar(ScreenState state, bool gamepadConnected);

    void drawPlayStationButton(int type, float cx, float cy, float radius);

    int windowWidth = 1280;
    int windowHeight = 720;
    int fbWidth = 1280;
    int fbHeight = 720;
    float dpiScale = 1.0f;

    float globalTime = 0.0f;
    float discSpinAngle = 0.0f;
    float discSpinSpeed = 110.0f;
    float gridScrollY = 0.0f;
    float targetGridScrollY = 0.0f;

    // The 7 Dancing Dots ("Seven Stars") parented to the spinning/precessing 3D circular curve
    struct StarOrb {
        uint32_t color;
        Vec3 currentPos;
        std::deque<Vec3> trail;
    };
    std::vector<StarOrb> sevenStars;
    Vec3 orbClusterCenter = {-1.35f, 0.05f, -5.5f};
    Vec3 targetClusterCenter = {-1.35f, 0.05f, -5.5f};

    struct MistTower {
        float x, y;
        float width, height;
        bool hollowTop = false;
    };
    std::vector<MistTower> bootTowers;
};
