#include "AudioEngine.hpp"
#include <iostream>
#include <cstdlib>
#include <cmath>

constexpr float TWO_PI = 6.28318530717958647692f;
constexpr int SAMPLE_RATE = 44100;

AudioEngine::AudioEngine() {}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init() {
    SDL_AudioSpec desired, obtained;
    SDL_zero(desired);
    desired.freq = SAMPLE_RATE;
    desired.format = AUDIO_F32SYS;
    desired.channels = 2;
    desired.samples = 1024;
    desired.callback = audioCallback;
    desired.userdata = this;

    deviceId = SDL_OpenAudioDevice(nullptr, 0, &desired, &obtained, 0);
    if (deviceId == 0) {
        std::cerr << "Failed to open audio: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_PauseAudioDevice(deviceId, 0);
    initialized = true;
    return true;
}

void AudioEngine::shutdown() {
    if (initialized && deviceId != 0) {
        SDL_CloseAudioDevice(deviceId);
        deviceId = 0;
        initialized = false;
    }
}

void AudioEngine::setAmbientEnabled(bool enabled) {
    std::lock_guard<std::mutex> lock(audioMutex);
    ambientEnabled = enabled;
}

void AudioEngine::setSoundMode(int mode) {
    std::lock_guard<std::mutex> lock(audioMutex);
    soundMode = mode;
}

void AudioEngine::setVolume(float volume) {
    std::lock_guard<std::mutex> lock(audioMutex);
    masterVolume = volume;
}

void AudioEngine::playSound(SoundEffect effect) {
    std::lock_guard<std::mutex> lock(audioMutex);

    switch (effect) {
        case SoundEffect::CURSOR_MOVE: {
            // PS2 crystalline metallic menu tick (two high harmonic glass partials with fast attack)
            activeTones.push_back({1480.0f, 1480.0f, 0.14f, 0.0f, 0.14f, 0, -0.15f, 0.0f});
            activeTones.push_back({2960.0f, 2960.0f, 0.09f, 0.0f, 0.08f, 0, 0.15f, 0.0f});
            activeTones.push_back({740.0f,  740.0f,  0.18f, 0.01f, 0.09f, 1, 0.0f, 0.0f});
            break;
        }
        case SoundEffect::ROTATE_WHOOSH: {
            // PS2 smooth crystalline glide
            activeTones.push_back({1108.73f, 1480.0f, 0.18f, 0.0f, 0.13f, 0, -0.2f, 0.0f});
            activeTones.push_back({2217.46f, 2960.0f, 0.14f, 0.02f, 0.07f, 0, 0.2f, 0.0f});
            break;
        }
        case SoundEffect::SELECT: {
            // PS2 Confirm Chime: Warm resonant fifth + glass bell shimmer
            activeTones.push_back({587.33f,  587.33f,  0.55f,  0.0f,  0.16f, 0, -0.25f, 0.0f}); // D5
            activeTones.push_back({880.00f,  880.00f,  0.65f, -0.03f, 0.18f, 0,  0.25f, 0.0f}); // A5
            activeTones.push_back({1174.66f, 1174.66f, 0.75f, -0.06f, 0.15f, 0, -0.10f, 0.0f}); // D6
            activeTones.push_back({1760.00f, 1760.00f, 0.85f, -0.09f, 0.10f, 0,  0.10f, 0.0f}); // A6
            break;
        }
        case SoundEffect::BACK: {
            // PS2 Cancel / Return: Descending hollow glass tone
            activeTones.push_back({880.00f, 880.00f, 0.28f,  0.0f,  0.15f, 0,  0.15f, 0.0f});
            activeTones.push_back({587.33f, 587.33f, 0.45f, -0.05f, 0.18f, 0, -0.15f, 0.0f});
            activeTones.push_back({293.66f, 293.66f, 0.50f, -0.07f, 0.12f, 1,  0.0f,  0.0f});
            break;
        }
        case SoundEffect::BOOT_JINGLE: {
            // Iconic PS2 Startup Sound:
            // 1. Deep atmospheric synth chord swell (D2, A2, E3, F#3, A3, C#4, E4, B4)
            float swellNotes[] = {73.42f, 110.00f, 164.81f, 220.00f, 277.18f, 329.63f, 493.88f, 659.25f};
            for (int i = 0; i < 8; i++) {
                float pan = (i % 2 == 0) ? -0.45f : 0.45f;
                activeTones.push_back({swellNotes[i] * 0.995f, swellNotes[i], 3.4f, -i * 0.06f, 0.11f, 0, pan, 0.0f});
            }
            // 2. High crystalline space wind chime at 1.4s
            float shimmer[] = {987.77f, 1318.51f, 1479.98f, 1975.53f, 2637.02f};
            for (int i = 0; i < 5; i++) {
                activeTones.push_back({shimmer[i], shimmer[i] * 1.002f, 2.2f, -1.2f - i * 0.12f, 0.08f, 0, (i % 2 == 0 ? -0.6f : 0.6f), 0.0f});
            }
            break;
        }
        case SoundEffect::DISC_SEEK: {
            // PS2 Disc spin-up + resonant launch chord
            activeTones.push_back({110.0f, 440.0f, 0.9f, 0.0f, 0.16f, 0, 0.0f, 0.0f});
            activeTones.push_back({440.0f, 880.0f, 1.1f, -0.15f, 0.18f, 0, -0.3f, 0.0f});
            activeTones.push_back({659.25f, 1318.51f, 1.2f, -0.25f, 0.16f, 0, 0.3f, 0.0f});
            break;
        }
        case SoundEffect::TEST_LEFT: {
            activeTones.push_back({523.25f, 523.25f, 0.55f, 0.0f, 0.28f, 0, -1.0f, 0.0f});
            activeTones.push_back({1046.50f, 1046.50f, 0.45f, 0.0f, 0.14f, 0, -1.0f, 0.0f});
            break;
        }
        case SoundEffect::TEST_RIGHT: {
            activeTones.push_back({659.25f, 659.25f, 0.55f, 0.0f, 0.28f, 0, 1.0f, 0.0f});
            activeTones.push_back({1318.51f, 1318.51f, 0.45f, 0.0f, 0.14f, 0, 1.0f, 0.0f});
            break;
        }
    }
}

void AudioEngine::audioCallback(void* userdata, Uint8* stream, int len) {
    AudioEngine* engine = static_cast<AudioEngine*>(userdata);
    float* output = reinterpret_cast<float*>(stream);
    int numFrames = len / (sizeof(float) * 2);
    engine->generateAudio(output, numFrames);
}

void AudioEngine::generateAudio(float* output, int numFrames) {
    std::lock_guard<std::mutex> lock(audioMutex);

    const float dt = 1.0f / SAMPLE_RATE;

    // PS2 Ambient Ethereal Chord + Ocean Surf Wave modulation
    const float ambFreqs[5] = {55.0f, 82.41f, 110.0f, 164.81f, 246.94f};

    for (int i = 0; i < numFrames; i++) {
        float sampleL = 0.0f;
        float sampleR = 0.0f;

        if (ambientEnabled) {
            // Slow ocean wave cycle (~0.14 Hz = 7 second ocean wave period)
            lfoPhase += TWO_PI * 0.14f * dt;
            if (lfoPhase > TWO_PI) lfoPhase -= TWO_PI;

            float waveEnv = 0.5f + 0.5f * std::sin(lfoPhase);
            float stereoDrift = std::cos(lfoPhase * 0.7f);

            // Soft filtered ocean surf noise
            float white = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
            float surf = white * (0.012f + 0.028f * waveEnv * waveEnv);

            // Ethereal low hollow pad
            float pad = 0.0f;
            for (int f = 0; f < 5; f++) {
                ambientPhase[f] += TWO_PI * ambFreqs[f] * dt;
                if (ambientPhase[f] > TWO_PI) ambientPhase[f] -= TWO_PI;
                pad += std::sin(ambientPhase[f]) * 0.025f;
            }

            float rawL = pad + surf * (1.0f + 0.3f * stereoDrift);
            float rawR = pad + surf * (1.0f - 0.3f * stereoDrift);

            // Dynamic low-pass filter that opens as the ocean wave crests
            float cutoffAlpha = 0.015f + 0.045f * waveEnv;
            filterL += cutoffAlpha * (rawL - filterL);
            filterR += cutoffAlpha * (rawR - filterR);

            sampleL += filterL;
            sampleR += filterR;
        }

        // Synthesized UI Tones
        for (auto it = activeTones.begin(); it != activeTones.end();) {
            it->time += dt;

            if (it->time < 0.0f) {
                ++it;
                continue;
            }

            if (it->time >= it->duration) {
                it = activeTones.erase(it);
                continue;
            }

            float progress = it->time / it->duration;
            float currentFreq = it->freqStart + (it->freqEnd - it->freqStart) * progress;

            it->phase += TWO_PI * currentFreq * dt;
            if (it->phase > TWO_PI) it->phase -= TWO_PI;

            float wave = 0.0f;
            if (it->waveType == 0) {
                wave = std::sin(it->phase);
            } else if (it->waveType == 1) {
                wave = 2.0f * std::fabs(2.0f * (it->phase / TWO_PI - std::floor(it->phase / TWO_PI + 0.5f))) - 1.0f;
            } else if (it->waveType == 2) {
                wave = (static_cast<float>(rand()) / static_cast<float>(RAND_MAX)) * 2.0f - 1.0f;
            } else {
                wave = it->phase < 3.14159f ? 1.0f : -1.0f;
            }

            // Smooth attack and long crystalline exponential decay
            float attack = std::min(1.0f, it->time / 0.012f);
            float decay = std::pow(1.0f - progress, 2.2f);
            float toneSample = wave * it->volume * attack * decay;

            float leftGain = 0.5f * (1.0f - it->pan);
            float rightGain = 0.5f * (1.0f + it->pan);

            sampleL += toneSample * leftGain;
            sampleR += toneSample * rightGain;

            ++it;
        }

        if (soundMode == 1) {
            float mono = (sampleL + sampleR) * 0.5f;
            sampleL = mono;
            sampleR = mono;
        } else if (soundMode == 2) {
            float diff = (sampleL - sampleR) * 0.4f;
            sampleL += diff;
            sampleR -= diff;
        }

        output[i * 2]     = std::clamp(sampleL * masterVolume, -0.98f, 0.98f);
        output[i * 2 + 1] = std::clamp(sampleR * masterVolume, -0.98f, 0.98f);
    }
}

