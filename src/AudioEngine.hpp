#pragma once

#include <SDL2/SDL.h>
#include <vector>
#include <mutex>
#include <cmath>

enum class SoundEffect {
    CURSOR_MOVE,
    ROTATE_WHOOSH,
    SELECT,
    BACK,
    BOOT_JINGLE,
    DISC_SEEK,
    TEST_LEFT,
    TEST_RIGHT
};

class AudioEngine {
public:
    AudioEngine();
    ~AudioEngine();

    bool init();
    void shutdown();

    void playSound(SoundEffect effect);
    void setAmbientEnabled(bool enabled);
    void setSoundMode(int mode);
    void setVolume(float volume);

private:
    static void audioCallback(void* userdata, Uint8* stream, int len);
    void generateAudio(float* output, int numFrames);

    SDL_AudioDeviceID deviceId = 0;
    bool initialized = false;
    bool ambientEnabled = true;
    int soundMode = 0;
    float masterVolume = 0.8f;

    std::mutex audioMutex;

    float ambientPhase[5] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
    float lfoPhase = 0.0f;
    float filterL = 0.0f;
    float filterR = 0.0f;

    struct ActiveTone {
        float freqStart;
        float freqEnd;
        float duration;
        float time;
        float volume;
        int waveType;
        float pan;
        float phase;
    };

    std::vector<ActiveTone> activeTones;
};

