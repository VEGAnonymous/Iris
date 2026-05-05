#pragma once

#include "Core/Defines.h"
#include "Core/Control/State/Envelope.h"

// Abstractions

struct Window {
    float start = 0.0f; // Normalized (up to MAX_IR_SAMPLES)
    float length = 1.0f;
    Envelope envelope;
};

struct RandomTimer {
    float minTime = SWAP_INTERVAL_MIN; // seconds
    float maxTime = SWAP_INTERVAL_MAX;
    float countdown = 0.0f;
    bool active = true;
    std::function<void()> callback;

    void advanceTimer(float dt) {
        if (!active) return;
        countdown -= dt;
        if (countdown <= 0.0f && callback) callback();
        // Callback should probably call resetCountdown()
    }
    void resetCountdown(juce::Random& rng) {
        countdown = active ? juce::jmap(rng.nextFloat(), minTime, maxTime) : 0.0f;
        DBG("SWAP: Reset countdown to " << countdown);
    }
};

// Enums

enum class IRSamplingMode {
    UNIFORM_ACROSS_ALL_FILES,
    UNIFORM_ACROSS_DIRECTORIES
};

// Data

struct IRSlot {
    juce::File file{};
    juce::AudioBuffer<float> buffer {};
    bool occupied = false;
    bool active = true;

    // Windowing
    Window window;
    inline float getMaxWindowLength() const {
        const int numSamples = buffer.getNumSamples();
        if (numSamples == 0) return 1.0f;
        else return juce::jlimit(0.0f, 1.0f, static_cast<float>(MAX_IR_SAMPLES) / static_cast<float>(numSamples));
    }

    // Auto swap at random intervals
    RandomTimer autoSwap;
};

struct IRSlotLite { // Extra thread-safe, bufferless
    juce::File file;
    bool occupied = false;
    bool active = true;

    Window window;
    float maxWindowLength = 1.0f;

    int numSamples = 0;
};

struct IRDirectory {
    juce::File irDirectory;
    bool active = true;
};

struct IRDirectoryFiles {
    IRDirectory dir;
    juce::Array<juce::File> files;
};

// Concurrency

struct IRCommand {
    enum CommandType {
        IR_LOAD,
        IR_LOAD_RANDOM,
        IR_LOAD_RANDOM_ALL,
        IR_CLEAR,
        IR_CLEAR_ALL,
        IR_SET_ACTIVE_STATE,
        IR_SET_WINDOW,
        IR_SET_ENVELOPE,
        IR_SET_SWAP_ACTIVE,
        IR_SET_SWAP_INTERVAL,

        IR_DIRECTORY_ADD,
        IR_DIRECTORY_REMOVE,
        IR_DIRECTORY_SET_ACTIVE_STATE,
        IR_DIRECTORY_COLLECT,
        IR_DIRECTORY_REFRESH,

        SET_FILE_FILTER,
        SET_DIRECTORY_FILTER,
        SET_SAMPLING_MODE
    } type = IR_CLEAR_ALL;

    int irIndex = 0;
    int irDirectoryIndex = 0;

    juce::File irFile;
    juce::File irDirectory;

    bool irActiveState = false;
    bool irDirectoryActiveState = false;

    float windowStart = 0.0f, windowLength = 0.0f;

    EnvelopeType envelopeType = EnvelopeType::NONE;
    float envelopeAttack = 0.0f, envelopeRelease = 0.0f;

    float swapMinTime = 0.0f, swapMaxTime = 0.0f;
    bool swapActiveState = false;

    juce::String fileFilter, directoryFilter;

    IRSamplingMode samplingMode = IRSamplingMode::UNIFORM_ACROSS_ALL_FILES;
};

struct IRResult {
    int irIndex = 0;
    juce::File file;
    juce::AudioBuffer<float> buffer{};
};

struct IRUpdate {
    enum Type {
        IR_SET,
        IR_CLEAR,
        IR_ACTIVE_CHANGED
    } type = IR_SET;

    int irIndex = 0;
};