#pragma once

#include <string>
#include <vector>
#include <cstdint>

enum class ScreenState {
    BOOT,
    MAIN_MENU,
    BROWSER_DEVICES,   // Level 1 Browser: Slot 1, Slot 2, PS2 Disc, Folder Mapper
    BROWSER_GRID,      // Level 2 Browser: 3D Grid of App/Save Icons inside Memory Card / Library
    BROWSER_ACTION,    // Action prompt on selected 3D icon (Launch / Reveal in Finder / Cancel)
    SYSTEM_CONFIG,     // PS2 System Configuration screen
    VERSION_INFO,      // PS2 Version Information (Triangle from Main Menu / System Config)
    DIR_MAPPING_MODAL  // Local Directory Mapper
};

enum class MenuTarget {
    BROWSER = 0,
    SYSTEM_CONFIG = 1
};

struct PS2App {
    std::string id;
    std::string title;
    std::string path;
    std::string category;
    std::string developer;
    uint32_t discColor; // 0xRRGGBB
    int saveBlocks;
};

struct GamepadToast {
    std::string title;
    std::string message;
    float timer;
    bool active;
};

struct SoundSettings {
    int mode;        // 0: Stereo, 1: Mono, 2: Digital Surround
    int screenSize;  // 0: 16:9, 1: 4:3, 2: Full
    bool opticalOut; // On / Off
    bool ambientMusic;
    bool sfxEnabled;
    float volume;
};
