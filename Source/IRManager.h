#pragma once

#include "Utilities.h"

#include <JuceHeader.h>

struct Envelope {
    EnvelopeType type = EnvelopeType::NONE;
    float attack = 0.0f, release = 0.0f; // Normalized (up to half window length)
    std::vector<float> curve {};
};

struct Window {
    // Normalized (up to MAX_IR_SAMPLES)
    float start = 0.0f;
    float length = 1.0f;
    Envelope envelope;
};

struct AutoSwap {
    float minTime = 0.0f, maxTime = 0.0f; // seconds
    float countdown = 0.0f;

    bool swapEnabled() const { return validateSwapInterval(minTime, maxTime); }
    void resetCountdown(juce::Random& rng) { countdown = juce::jmap(rng.nextFloat(), minTime, maxTime); }
};

struct IRSlot {
    juce::File file{};
    juce::AudioBuffer<float> buffer{};
    bool occupied = false;
    bool active = true;

    // Windowing
    Window window;

    // Self-swap
    AutoSwap autoSwap;
};

struct IRDirectory {
    juce::File irDirectory;
    bool active = true;
};

struct IRChanges {
    std::deque<int> irsSet {};
    std::deque<int> irsCleared {};
    std::deque<int> irsActiveStateSet {};
};

class IRManager {
private:
    juce::ApplicationProperties* applicationProperties;

    std::array<IRSlot, MAX_IR_COUNT> irSlots;

    std::vector<IRDirectory> irDirectories;
    juce::Array<juce::File> irFiles;

    IRChanges irChanges;
    std::atomic<bool> directoryChanged { false };

    juce::AudioFormatManager formatManager;
    juce::Random irRNG;
    std::unique_ptr<juce::FileChooser> irFileChooser, irDirectoryChooser;

    void saveDirectories();
    void loadDirectories();

    void collectIRs();

    void IRManager::computeEnvelope(IRSlot& slot);

public:
    IRManager(juce::ApplicationProperties* p);
    ~IRManager() = default;

    void prepare();

    void chooseIR(int irIndex);
    void chooseIRDirectory();

    void addIRDirectory(juce::File dir);
    void removeIRDirectory(int index);
    void setIRDirectoryActive(int index, bool nState);

    bool loadIR(int irIndex, juce::File file);
    bool loadRandomIR(int irIndex);
    bool loadRandomIRs();
    void clearIR(int irIndex);
    void clearIRs();

    void setIRActive(int irIndex, bool nState);

    bool setWindow(int irIndex, float start, float length);
    void setEnvelope(int irIndex, EnvelopeType type, float attack, float release);

    juce::AudioBuffer<float> applyWindow(int irIndex) const;

    void setSwapInterval(int irIndex, float minTime, float maxTime);
    void advanceSwapTimers(float dt);

    IRChanges consumeIRChanges();
    std::atomic<bool>& getDirectoryChanged();

    const IRSlot& getIRSlot(int index) const;
    const std::vector<IRDirectory> getIRDirectories() const;
};