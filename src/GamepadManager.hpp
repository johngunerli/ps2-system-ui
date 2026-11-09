#pragma once

#include <SDL2/SDL.h>
#include <string>
#include <vector>
#include "Types.hpp"

class GamepadManager {
public:
    GamepadManager();
    ~GamepadManager();

    void init();
    void update(float deltaTime);
    void handleEvent(const SDL_Event& event);

    bool isConnected() const;
    std::string getControllerName() const;

    bool wasJustPressed(SDL_GameControllerButton button);
    bool isDown(SDL_GameControllerButton button);

    bool wasDirUpPressed();
    bool wasDirDownPressed();
    bool wasDirLeftPressed();
    bool wasDirRightPressed();

    bool hasActiveToast() const;
    GamepadToast getActiveToast() const;

private:
    void openController(int deviceIndex);
    void closeController();

    SDL_GameController* controller = nullptr;
    int controllerIndex = -1;
    std::string controllerName;

    bool currentButtons[SDL_CONTROLLER_BUTTON_MAX];
    bool previousButtons[SDL_CONTROLLER_BUTTON_MAX];

    float stickX = 0.0f;
    float stickY = 0.0f;
    float lastStickTriggerTime = 0.0f;

    GamepadToast activeToast;
};

