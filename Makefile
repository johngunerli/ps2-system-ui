CXX = clang++
CXXFLAGS = -std=c++20 -O2 -Wall -Wextra -DGL_SILENCE_DEPRECATION -I/opt/homebrew/include -Isrc
LDFLAGS = -L/opt/homebrew/lib -lSDL2 -framework OpenGL -framework Cocoa -framework IOKit -framework CoreVideo -framework CoreGraphics -framework CoreText -framework CoreFoundation

SRCS = src/main.cpp \
       src/TextRenderer.cpp \
       src/AudioEngine.cpp \
       src/GamepadManager.cpp \
       src/AppScanner.cpp \
       src/Renderer.cpp

OBJS = $(SRCS:.cpp=.o)
TARGET = ps2
APP_BUNDLE = PS2.app

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

bundle: $(TARGET)
	@echo "Creating macOS application bundle: $(APP_BUNDLE)..."
	@mkdir -p $(APP_BUNDLE)/Contents/MacOS
	@mkdir -p $(APP_BUNDLE)/Contents/Resources
	@cp $(TARGET) $(APP_BUNDLE)/Contents/MacOS/$(TARGET)
	@echo '<?xml version="1.0" encoding="UTF-8"?>\
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\
<plist version="1.0">\
<dict>\
    <key>CFBundleExecutable</key>\
    <string>ps2</string>\
    <key>CFBundleIdentifier</key>\
    <string>com.retro.ps2-system-ui</string>\
    <key>CFBundleName</key>\
    <string>PS2 System Menu</string>\
    <key>CFBundlePackageType</key>\
    <string>APPL</string>\
    <key>CFBundleShortVersionString</key>\
    <string>2.0</string>\
    <key>LSMinimumSystemVersion</key>\
    <string>11.0</string>\
    <key>NSHighResolutionCapable</key>\
    <true/>\
</dict>\
</plist>' > $(APP_BUNDLE)/Contents/Info.plist
	@echo "Bundle created at $(APP_BUNDLE). You can drag this into /Applications!"

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(OBJS) $(TARGET) gamecube src/Font.o
	rm -rf $(APP_BUNDLE) GameCube.app

.PHONY: all bundle run clean

