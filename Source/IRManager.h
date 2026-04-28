#pragma once

#include "Envelope.h"
#include "Utilities.h"

#include <JuceHeader.h>

// Leaving these here for now but may move to separate headers later /*

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
    }
};

// */

struct IRSlot {
    juce::File file{};
    juce::AudioBuffer<float> buffer{};
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
    void setSwapActive(int irIndex, bool nActive);
    void advanceSwapTimers(float dt);

    IRChanges consumeIRChanges();
    std::atomic<bool>& getDirectoryChanged();

    const IRSlot& getIRSlot(int index) const;
    const std::vector<IRDirectory> getIRDirectories() const;

    juce::AudioFormatManager* getFormatManager();
};