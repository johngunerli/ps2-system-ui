#include "AppScanner.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <unistd.h>
#include <pwd.h>

namespace fs = std::filesystem;

AppScanner::AppScanner() {
    const char* home = std::getenv("HOME");
    if (!home) {
        struct passwd* pw = getpwuid(getuid());
        if (pw) home = pw->pw_dir;
    }

    if (home) {
        configFilePath = std::string(home) + "/.ps2_ui.cfg";
    } else {
        configFilePath = "ps2_ui.cfg";
    }
}

void AppScanner::init() {
    loadConfig();

    if (mappedDirectories.empty()) {
        if (fs::exists("/Applications")) {
            mappedDirectories.push_back("/Applications");
        }
        const char* home = std::getenv("HOME");
        if (home) {
            std::string userApps = std::string(home) + "/Applications";
            if (fs::exists(userApps)) {
                mappedDirectories.push_back(userApps);
            }
            std::string userGames = std::string(home) + "/Games";
            if (fs::exists(userGames)) {
                mappedDirectories.push_back(userGames);
            }
        }
        saveConfig();
    }

    scanAll();
}

void AppScanner::loadConfig() {
    mappedDirectories.clear();
    std::ifstream inFile(configFilePath);
    if (!inFile.is_open()) return;

    std::string line;
    while (std::getline(inFile, line)) {
        if (!line.empty() && line[0] != '#') {
            mappedDirectories.push_back(line);
        }
    }
}

void AppScanner::saveConfig() {
    std::ofstream outFile(configFilePath);
    if (!outFile.is_open()) return;

    outFile << "# PlayStation 2 UI Mapped Directories\n";
    for (const auto& dir : mappedDirectories) {
        outFile << dir << "\n";
    }
}

void AppScanner::addDirectory(const std::string& path) {
    if (path.empty()) return;
    std::string cleanPath = path;
    while (cleanPath.size() > 1 && cleanPath.back() == '/') {
        cleanPath.pop_back();
    }
    for (const auto& existing : mappedDirectories) {
        if (existing == cleanPath) return;
    }
    mappedDirectories.push_back(cleanPath);
    saveConfig();
    scanAll();
}

void AppScanner::removeDirectory(size_t index) {
    if (index < mappedDirectories.size()) {
        mappedDirectories.erase(mappedDirectories.begin() + index);
        saveConfig();
        scanAll();
    }
}

bool AppScanner::pickAndAddDirectoryWithDialog() {
#ifdef __APPLE__
    FILE* pipe = popen("osascript -e 'POSIX path of (choose folder with prompt \"Select a directory to mount in PlayStation 2 Browser:\")' 2>/dev/null", "r");
    if (!pipe) return false;
    char buffer[1024];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    int status = pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    if (status == 0 && !result.empty()) {
        addDirectory(result);
        return true;
    }
#endif
    return false;
}

const std::vector<std::string>& AppScanner::getMappedDirectories() const {
    return mappedDirectories;
}

const std::vector<PS2App>& AppScanner::getDiscs() const {
    return discoveredDiscs;
}

uint32_t AppScanner::generateDiscColor(const std::string& name) {
    uint32_t hash = 5381;
    for (char c : name) {
        hash = ((hash << 5) + hash) + static_cast<uint8_t>(c);
    }

    const uint32_t palette[] = {
        0x2962FF, // PS2 Royal Blue
        0x00B0FF, // PS2 Cyan
        0xFFD54F, // DVD-ROM Gold
        0x536DFE, // Deep Indigo
        0x00E5FF, // Electric Aqua
        0x7C4DFF, // Violet
        0xFF5252, // Crimson
        0x1DE9B6  // Emerald
    };

    return palette[hash % (sizeof(palette) / sizeof(palette[0]))];
}

void AppScanner::scanDirectory(const std::string& dirPath) {
    try {
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath)) return;

        for (const auto& entry : fs::directory_iterator(dirPath, fs::directory_options::skip_permission_denied)) {
            std::string pathStr = entry.path().string();
            std::string ext = entry.path().extension().string();
            for (char& c : ext) c = std::tolower(c);

            bool isApp = false;
            std::string category = "PS2 APPLICATION";
            std::string developer = "LOCAL SYSTEM APP";

            if (ext == ".app") {
                isApp = true;
                category = "MACOS APPLICATION";
                developer = "PLAYSTATION 2 / MACOS";
            } else if (ext == ".iso" || ext == ".bin" || ext == ".cue" || ext == ".chd" || ext == ".elf" || ext == ".gcm" || ext == ".rvz") {
                isApp = true;
                category = "PS2 FORMAT DVD-ROM";
                developer = "SONY COMPUTER ENTERTAINMENT";
            }

            if (isApp) {
                PS2App app;
                app.id = pathStr;
                app.path = pathStr;
                app.category = category;
                app.developer = developer;

                std::string title = entry.path().stem().string();
                app.title = title;
                app.discColor = generateDiscColor(title);
                app.saveBlocks = 4 + (title.length() % 18);

                discoveredDiscs.push_back(app);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error scanning directory " << dirPath << ": " << e.what() << std::endl;
    }
}

void AppScanner::scanAll() {
    discoveredDiscs.clear();

    for (const auto& dir : mappedDirectories) {
        scanDirectory(dir);
    }

    std::sort(discoveredDiscs.begin(), discoveredDiscs.end(), [](const PS2App& a, const PS2App& b) {
        return a.title < b.title;
    });

    if (discoveredDiscs.empty()) {
        discoveredDiscs.push_back({
            "sample-gt3", "Gran Turismo 3: A-Spec", "/Applications",
            "PS2 FORMAT DVD-ROM", "Polyphony Digital / SCEI", 0x2962FF, 14
        });
        discoveredDiscs.push_back({
            "sample-sh2", "Silent Hill 2", "/Applications",
            "PS2 FORMAT DVD-ROM", "Konami Computer Entertainment", 0x00B0FF, 9
        });
        discoveredDiscs.push_back({
            "sample-mgs2", "Metal Gear Solid 2: Sons of Liberty", "/Applications",
            "PS2 FORMAT DVD-ROM", "Konami / Kojima Productions", 0xFFD54F, 12
        });
    }

    std::cout << "[AppScanner] Discovered " << discoveredDiscs.size() << " titles across mapped directories." << std::endl;
}

bool AppScanner::launchApp(const PS2App& app) {
    if (app.path.empty()) return false;

    std::string cmd;
#ifdef __APPLE__
    cmd = "open \"" + app.path + "\" &";
#else
    cmd = "\"" + app.path + "\" &";
#endif

    std::cout << "[AppScanner] Launching: " << cmd << std::endl;
    int ret = std::system(cmd.c_str());
    return ret == 0;
}

