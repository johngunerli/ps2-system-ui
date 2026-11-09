#include "GamepadManager.hpp"
#include <iostream>
#include <cstring>
#include <cmath>

constexpr float STICK_DEADZONE = 0.35f;
constexpr float STICK_REPEAT_DELAY = 0.20f;

GamepadManager::GamepadManager() {
    std::memset(currentButtons, 0, sizeof(currentButtons));
    std::memset(previousButtons, 0, sizeof(previousButtons));
    activeToast.active = false;
    activeToast.timer = 0.0f;
}

GamepadManager::~GamepadManager() {
    closeController();
}

void GamepadManager::init() {
    int numJoysticks = SDL_NumJoysticks();
    for (int i = 0; i < numJoysticks; ++i) {
        if (SDL_IsGameController(i)) {
            openController(i);
            break;
        }
    }
}

void GamepadManager::openController(int deviceIndex) {
    closeController();

    controller = SDL_GameControllerOpen(deviceIndex);
    if (controller) {
        controllerIndex = deviceIndex;
        const char* name = SDL_GameControllerName(controller);
        controllerName = name ? name : "DualShock / Controller";

        activeToast.title = "CONTROLLER CONNECTED (PORT " + std::to_string(deviceIndex + 1) + ")";
        activeToast.message = controllerName.substr(0, 36);
        activeToast.timer = 5.0f;
        activeToast.active = true;

        std::cout << "[Gamepad] Connected: " << controllerName << " on Port " << (deviceIndex + 1) << std::endl;
    }
}

void GamepadManager::closeController() {
    if (controller) {
        activeToast.title = "CONTROLLER REMOVED";
        activeToast.message = "Port " + std::to_string(controllerIndex + 1) + " disconnected";
        activeToast.timer = 4.0f;
        activeToast.active = true;

        SDL_GameControllerClose(controller);
        controller = nullptr;
        controllerIndex = -1;
        controllerName.clear();
    }
}

void GamepadManager::handleEvent(const SDL_Event& event) {
    if (event.type == SDL_CONTROLLERDEVICEADDED) {
        if (!controller) {
            openController(event.cdevice.which);
        }
    } else if (event.type == SDL_CONTROLLERDEVICEREMOVED) {
        if (controller && event.cdevice.which == SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller))) {
            closeController();
        }
    }
}

void GamepadManager::update(float deltaTime) {
    std::memcpy(previousButtons, currentButtons, sizeof(currentButtons));

    if (activeToast.active) {
        activeToast.timer -= deltaTime;
        if (activeToast.timer <= 0.0f) {
            activeToast.active = false;
        }
    }

    if (lastStickTriggerTime > 0.0f) {
        lastStickTriggerTime -= deltaTime;
    }

    if (!controller) return;

    for (int b = 0; b < SDL_CONTROLLER_BUTTON_MAX; ++b) {
        currentButtons[b] = SDL_GameControllerGetButton(controller, static_cast<SDL_GameControllerButton>(b)) != 0;
    }

    Sint16 rawX = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
    Sint16 rawY = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);

    stickX = rawX / 32767.0f;
    stickY = rawY / 32767.0f;
}

bool GamepadManager::isConnected() const {
    return controller != nullptr;
}

std::string GamepadManager::getControllerName() const {
    return controllerName;
}

bool GamepadManager::wasJustPressed(SDL_GameControllerButton button) {
    if (button < 0 || button >= SDL_CONTROLLER_BUTTON_MAX) return false;
    return currentButtons[button] && !previousButtons[button];
}

bool GamepadManager::isDown(SDL_GameControllerButton button) {
    if (button < 0 || button >= SDL_CONTROLLER_BUTTON_MAX) return false;
    return currentButtons[button];
}

bool GamepadManager::wasDirUpPressed() {
    if (wasJustPressed(SDL_CONTROLLER_BUTTON_DPAD_UP)) return true;
    if (stickY < -STICK_DEADZONE && lastStickTriggerTime <= 0.0f) {
        lastStickTriggerTime = STICK_REPEAT_DELAY;
        return true;
    }
    return false;
}

bool GamepadManager::wasDirDownPressed() {
    if (wasJustPressed(SDL_CONTROLLER_BUTTON_DPAD_DOWN)) return true;
    if (stickY > STICK_DEADZONE && lastStickTriggerTime <= 0.0f) {
        lastStickTriggerTime = STICK_REPEAT_DELAY;
        return true;
    }
    return false;
}

bool GamepadManager::wasDirLeftPressed() {
    if (wasJustPressed(SDL_CONTROLLER_BUTTON_DPAD_LEFT)) return true;
    if (stickX < -STICK_DEADZONE && lastStickTriggerTime <= 0.0f) {
        lastStickTriggerTime = STICK_REPEAT_DELAY;
        return true;
    }
    return false;
}

bool GamepadManager::wasDirRightPressed() {
    if (wasJustPressed(SDL_CONTROLLER_BUTTON_DPAD_RIGHT)) return true;
    if (stickX > STICK_DEADZONE && lastStickTriggerTime <= 0.0f) {
        lastStickTriggerTime = STICK_REPEAT_DELAY;
        return true;
    }
    return false;
}

bool GamepadManager::hasActiveToast() const {
    return activeToast.active;
}

GamepadToast GamepadManager::getActiveToast() const {
    return activeToast;
}

