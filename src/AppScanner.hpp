#pragma once

#include <string>
#include <vector>
#include "Types.hpp"

class AppScanner {
public:
    AppScanner();

    void init();
    void scanAll();
    void addDirectory(const std::string& path);
    void removeDirectory(size_t index);
    bool pickAndAddDirectoryWithDialog();

    const std::vector<std::string>& getMappedDirectories() const;
    const std::vector<PS2App>& getDiscs() const;

    bool launchApp(const PS2App& app);

    void saveConfig();
    void loadConfig();

private:
    void scanDirectory(const std::string& dirPath);
    uint32_t generateDiscColor(const std::string& name);

    std::vector<std::string> mappedDirectories;
    std::vector<PS2App> discoveredDiscs;
    std::string configFilePath;
};

