#include <SDL2/SDL.h>
#include <OpenGL/gl.h>
#include <iostream>
#include <vector>
#include <string>
#include <cstdlib>

#include "Types.hpp"
#include "TextRenderer.hpp"
#include "AudioEngine.hpp"
#include "GamepadManager.hpp"
#include "AppScanner.hpp"
#include "Renderer.hpp"

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    std::cout << "Starting PlayStation 2 BIOS System Menu (C++20 Native)..." << std::endl;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER | SDL_INIT_JOYSTICK) != 0) {
        std::cerr << "Failed to initialize SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    int winWidth = 1280;
    int winHeight = 720;
    SDL_Window* window = SDL_CreateWindow(
        "PlayStation 2",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        winWidth,
        winHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
    );

    if (!window) {
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 0);
        SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 0);
        window = SDL_CreateWindow(
            "PlayStation 2",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            winWidth,
            winHeight,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI
        );
    }

    if (!window) {
        std::cerr << "Failed to create SDL window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "Failed to create OpenGL context: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_GL_SetSwapInterval(1);
    glEnable(GL_MULTISAMPLE);

    int drawableW = winWidth, drawableH = winHeight;
    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);

    AudioEngine audioEngine;
    audioEngine.init();

    GamepadManager gamepadManager;
    gamepadManager.init();

    AppScanner appScanner;
    appScanner.init();

    Renderer renderer;
    renderer.init(winWidth, winHeight, drawableW, drawableH);

    ScreenState currentState = ScreenState::BOOT;
    ScreenState returnStateFromVersion = ScreenState::MAIN_MENU;
    MenuTarget currentTarget = MenuTarget::BROWSER;

    SoundSettings soundSettings;
    soundSettings.mode = 0;       // Stereo
    soundSettings.screenSize = 0; // 16:9
    soundSettings.opticalOut = true;
    soundSettings.ambientMusic = true;
    soundSettings.sfxEnabled = true;
    soundSettings.volume = 0.85f;

    auto applyScreenSize = [&](int mode) {
        if (mode == 0) {
            // 16:9 Widescreen
            SDL_SetWindowFullscreen(window, 0);
            SDL_SetWindowSize(window, 1280, 720);
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        } else if (mode == 1) {
            // 4:3 Standard CRT Aspect Ratio
            SDL_SetWindowFullscreen(window, 0);
            SDL_SetWindowSize(window, 960, 720);
            SDL_SetWindowPosition(window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
        } else if (mode == 2) {
            // Fullscreen
            SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
        }
        SDL_GetWindowSize(window, &winWidth, &winHeight);
        SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
        renderer.resize(winWidth, winHeight, drawableW, drawableH);
    };

    int selectedDeviceIndex = 0; // 0: Slot 1, 1: Slot 2, 2: PS2 Disc, 3: Local Directory Mounts
    size_t selectedAppIndex = 0;
    int selectedActionIndex = 0; // 0: Launch, 1: Reveal in Finder, 2: Cancel
    size_t modalDirIndex = 0;
    int activeMemorySlot = 0;
    int selectedOptionIndex = 0;
    bool nextTestRightSpeaker = false;

    float bootProgress = 0.0f;
    audioEngine.playSound(SoundEffect::BOOT_JINGLE);

    Uint32 lastTicks = SDL_GetTicks();
    bool running = true;

    while (running) {
        Uint32 currentTicks = SDL_GetTicks();
        float deltaTime = (currentTicks - lastTicks) / 1000.0f;
        if (deltaTime > 0.1f) deltaTime = 0.1f;
        lastTicks = currentTicks;

        bool mouseClicked = false;
        int mouseWheelY = 0;

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_WINDOWEVENT) {
                if (event.window.event == SDL_WINDOWEVENT_RESIZED ||
                    event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    SDL_GetWindowSize(window, &winWidth, &winHeight);
                    SDL_GL_GetDrawableSize(window, &drawableW, &drawableH);
                    renderer.resize(winWidth, winHeight, drawableW, drawableH);
                }
            } else if (event.type == SDL_CONTROLLERDEVICEADDED || event.type == SDL_CONTROLLERDEVICEREMOVED) {
                gamepadManager.handleEvent(event);
                audioEngine.playSound(SoundEffect::SELECT);
            } else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
                mouseClicked = true;
            } else if (event.type == SDL_MOUSEMOTION) {
                int mx = event.motion.x;
                int my = event.motion.y;
                if (currentState == ScreenState::MAIN_MENU) {
                    int hit = renderer.hitTestMainMenu(mx, my);
                    if (hit == 0 && currentTarget != MenuTarget::BROWSER) {
                        currentTarget = MenuTarget::BROWSER;
                        audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                    } else if (hit == 1 && currentTarget != MenuTarget::SYSTEM_CONFIG) {
                        currentTarget = MenuTarget::SYSTEM_CONFIG;
                        audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                    }
                } else if (currentState == ScreenState::BROWSER_DEVICES) {
                    int hit = renderer.hitTestBrowserDevices(mx, my);
                    if (hit >= 0 && hit != selectedDeviceIndex) {
                        selectedDeviceIndex = hit;
                        audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                    }
                } else if (currentState == ScreenState::BROWSER_GRID) {
                    int hit = renderer.hitTestBrowserGrid(mx, my, appScanner.getDiscs().size(), selectedAppIndex);
                    if (hit >= 0 && static_cast<size_t>(hit) != selectedAppIndex) {
                        selectedAppIndex = static_cast<size_t>(hit);
                        audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                    }
                } else if (currentState == ScreenState::SYSTEM_CONFIG) {
                    int hit = renderer.hitTestSystemConfig(mx, my);
                    if (hit >= 0 && hit != selectedOptionIndex) {
                        selectedOptionIndex = hit;
                        audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                    }
                }
            } else if (event.type == SDL_MOUSEWHEEL) {
                mouseWheelY = event.wheel.y;
            }
        }

        gamepadManager.update(deltaTime);

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        static bool prevUp = false, prevDown = false, prevLeft = false, prevRight = false;
        static bool prevEnter = false, prevEsc = false, prevKeyX = false, prevKeyY = false;

        bool currUp = keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W];
        bool currDown = keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S];
        bool currLeft = keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A];
        bool currRight = keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D];
        bool currEnter = keys[SDL_SCANCODE_RETURN] || keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_Z];
        bool currEsc = keys[SDL_SCANCODE_ESCAPE] || keys[SDL_SCANCODE_BACKSPACE] || keys[SDL_SCANCODE_X];
        bool currKeyX = keys[SDL_SCANCODE_C];
        bool currKeyY = keys[SDL_SCANCODE_V];

        bool justUp = (currUp && !prevUp) || gamepadManager.wasDirUpPressed() || (mouseWheelY > 0);
        bool justDown = (currDown && !prevDown) || gamepadManager.wasDirDownPressed() || (mouseWheelY < 0);
        bool justLeft = (currLeft && !prevLeft) || gamepadManager.wasDirLeftPressed();
        bool justRight = (currRight && !prevRight) || gamepadManager.wasDirRightPressed();
        bool justCross = (currEnter && !prevEnter) || gamepadManager.wasJustPressed(SDL_CONTROLLER_BUTTON_A) || mouseClicked;
        bool justCircle = (currEsc && !prevEsc) || gamepadManager.wasJustPressed(SDL_CONTROLLER_BUTTON_B);
        bool justSquare = (currKeyX && !prevKeyX) || gamepadManager.wasJustPressed(SDL_CONTROLLER_BUTTON_X);
        bool justTriangle = (currKeyY && !prevKeyY) || gamepadManager.wasJustPressed(SDL_CONTROLLER_BUTTON_Y);
        bool justStart = gamepadManager.wasJustPressed(SDL_CONTROLLER_BUTTON_START);

        prevUp = currUp; prevDown = currDown; prevLeft = currLeft; prevRight = currRight;
        prevEnter = currEnter; prevEsc = currEsc; prevKeyX = currKeyX; prevKeyY = currKeyY;

        if (currentState == ScreenState::BOOT) {
            bootProgress += deltaTime / 6.2f;
            if (justCross || justStart || bootProgress >= 1.0f) {
                currentState = ScreenState::MAIN_MENU;
                audioEngine.playSound(SoundEffect::SELECT);
            }
        }
        else if (currentState == ScreenState::MAIN_MENU) {
            if (justUp || justDown) {
                currentTarget = (currentTarget == MenuTarget::BROWSER)
                                ? MenuTarget::SYSTEM_CONFIG
                                : MenuTarget::BROWSER;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justTriangle) {
                audioEngine.playSound(SoundEffect::SELECT);
                returnStateFromVersion = ScreenState::MAIN_MENU;
                currentState = ScreenState::VERSION_INFO;
            } else if (justCross) {
                audioEngine.playSound(SoundEffect::SELECT);
                if (currentTarget == MenuTarget::BROWSER) {
                    currentState = ScreenState::BROWSER_DEVICES;
                } else {
                    currentState = ScreenState::SYSTEM_CONFIG;
                }
            }
        }
        else if (currentState == ScreenState::BROWSER_DEVICES) {
            if (justLeft && selectedDeviceIndex > 0) {
                selectedDeviceIndex--;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justRight && selectedDeviceIndex < 3) {
                selectedDeviceIndex++;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justTriangle) {
                audioEngine.playSound(SoundEffect::SELECT);
                currentState = ScreenState::DIR_MAPPING_MODAL;
            } else if (justCross) {
                audioEngine.playSound(SoundEffect::SELECT);
                if (selectedDeviceIndex == 0 || selectedDeviceIndex == 1) {
                    activeMemorySlot = selectedDeviceIndex;
                    currentState = ScreenState::BROWSER_GRID;
                } else if (selectedDeviceIndex == 2) {
                    // Quick Launch active PS2 Format Disc or open Grid
                    const auto& discs = appScanner.getDiscs();
                    if (!discs.empty()) {
                        audioEngine.playSound(SoundEffect::DISC_SEEK);
                        renderer.triggerDiscLaunchSpin();
                        appScanner.launchApp(discs[selectedAppIndex]);
                    }
                } else {
                    currentState = ScreenState::DIR_MAPPING_MODAL;
                }
            } else if (justCircle) {
                audioEngine.playSound(SoundEffect::BACK);
                currentState = ScreenState::MAIN_MENU;
            }
        }
        else if (currentState == ScreenState::BROWSER_GRID) {
            const auto& discs = appScanner.getDiscs();
            if (!discs.empty()) {
                if (justLeft && selectedAppIndex > 0) {
                    selectedAppIndex--;
                    audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                } else if (justRight && selectedAppIndex + 1 < discs.size()) {
                    selectedAppIndex++;
                    audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                } else if (justUp && selectedAppIndex >= 4) {
                    selectedAppIndex -= 4;
                    audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                } else if (justDown && selectedAppIndex + 4 < discs.size()) {
                    selectedAppIndex += 4;
                    audioEngine.playSound(SoundEffect::CURSOR_MOVE);
                } else if (justCross) {
                    selectedActionIndex = 0;
                    audioEngine.playSound(SoundEffect::SELECT);
                    currentState = ScreenState::BROWSER_ACTION;
                }
            }
            if (justTriangle) {
                audioEngine.playSound(SoundEffect::SELECT);
                currentState = ScreenState::DIR_MAPPING_MODAL;
            } else if (justCircle) {
                audioEngine.playSound(SoundEffect::BACK);
                currentState = ScreenState::BROWSER_DEVICES;
            }
        }
        else if (currentState == ScreenState::BROWSER_ACTION) {
            const auto& discs = appScanner.getDiscs();
            if (justUp && selectedActionIndex > 0) {
                selectedActionIndex--;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justDown && selectedActionIndex < 2) {
                selectedActionIndex++;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justCross && !discs.empty() && selectedAppIndex < discs.size()) {
                if (selectedActionIndex == 0) {
                    audioEngine.playSound(SoundEffect::DISC_SEEK);
                    renderer.triggerDiscLaunchSpin();
                    appScanner.launchApp(discs[selectedAppIndex]);
                    currentState = ScreenState::BROWSER_GRID;
                } else if (selectedActionIndex == 1) {
                    audioEngine.playSound(SoundEffect::SELECT);
                    std::string revealCmd = "open -R \"" + discs[selectedAppIndex].path + "\" &";
                    std::system(revealCmd.c_str());
                    currentState = ScreenState::BROWSER_GRID;
                } else {
                    audioEngine.playSound(SoundEffect::BACK);
                    currentState = ScreenState::BROWSER_GRID;
                }
            } else if (justCircle) {
                audioEngine.playSound(SoundEffect::BACK);
                currentState = ScreenState::BROWSER_GRID;
            }
        }
        else if (currentState == ScreenState::SYSTEM_CONFIG) {
            if (justUp && selectedOptionIndex > 0) {
                selectedOptionIndex--;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justDown && selectedOptionIndex < 5) {
                selectedOptionIndex++;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justTriangle) {
                audioEngine.playSound(SoundEffect::SELECT);
                returnStateFromVersion = ScreenState::SYSTEM_CONFIG;
                currentState = ScreenState::VERSION_INFO;
            } else if (justCross || justLeft || justRight) {
                int step = justLeft ? 2 : 1; // +2 mod 3 is -1, +1 mod 3 is +1
                if (selectedOptionIndex == 0) {
                    audioEngine.playSound(SoundEffect::SELECT);
                } else if (selectedOptionIndex == 1) {
                    soundSettings.screenSize = (soundSettings.screenSize + step) % 3;
                    applyScreenSize(soundSettings.screenSize);
                    audioEngine.playSound(SoundEffect::SELECT);
                } else if (selectedOptionIndex == 2) {
                    soundSettings.opticalOut = !soundSettings.opticalOut;
                    audioEngine.playSound(SoundEffect::SELECT);
                } else if (selectedOptionIndex == 3) {
                    soundSettings.mode = (soundSettings.mode + step) % 3;
                    audioEngine.setSoundMode(soundSettings.mode);
                    audioEngine.playSound(SoundEffect::SELECT);
                } else if (selectedOptionIndex == 4) {
                    soundSettings.ambientMusic = !soundSettings.ambientMusic;
                    audioEngine.setAmbientEnabled(soundSettings.ambientMusic);
                    audioEngine.playSound(SoundEffect::SELECT);
                } else if (selectedOptionIndex == 5) {
                    audioEngine.playSound(nextTestRightSpeaker ? SoundEffect::TEST_RIGHT : SoundEffect::TEST_LEFT);
                    nextTestRightSpeaker = !nextTestRightSpeaker;
                }
            } else if (justCircle) {
                audioEngine.playSound(SoundEffect::BACK);
                currentState = ScreenState::MAIN_MENU;
            }
        }
        else if (currentState == ScreenState::VERSION_INFO) {
            if (justCircle || justCross || justTriangle) {
                audioEngine.playSound(SoundEffect::BACK);
                currentState = returnStateFromVersion;
            }
        }
        else if (currentState == ScreenState::DIR_MAPPING_MODAL) {
            const auto& dirs = appScanner.getMappedDirectories();
            if (justUp && modalDirIndex > 0) {
                modalDirIndex--;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justDown && !dirs.empty() && modalDirIndex + 1 < dirs.size()) {
                modalDirIndex++;
                audioEngine.playSound(SoundEffect::CURSOR_MOVE);
            } else if (justCross) {
                audioEngine.playSound(SoundEffect::SELECT);
                appScanner.scanAll();
                selectedAppIndex = 0;
            } else if (justTriangle) {
                audioEngine.playSound(SoundEffect::SELECT);
                if (appScanner.pickAndAddDirectoryWithDialog()) {
                    audioEngine.playSound(SoundEffect::SELECT);
                    selectedAppIndex = 0;
                }
            } else if (justSquare && dirs.size() > 1) {
                appScanner.removeDirectory(modalDirIndex);
                if (modalDirIndex >= appScanner.getMappedDirectories().size() && modalDirIndex > 0) {
                    modalDirIndex--;
                }
                audioEngine.playSound(SoundEffect::BACK);
            } else if (justCircle) {
                audioEngine.playSound(SoundEffect::BACK);
                currentState = ScreenState::BROWSER_DEVICES;
            }
        }

        renderer.render(
            deltaTime,
            currentState,
            currentTarget,
            bootProgress,
            appScanner,
            selectedDeviceIndex,
            selectedAppIndex,
            selectedActionIndex,
            modalDirIndex,
            gamepadManager,
            soundSettings,
            activeMemorySlot,
            selectedOptionIndex
        );

        SDL_GL_SwapWindow(window);
    }

    audioEngine.shutdown();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
