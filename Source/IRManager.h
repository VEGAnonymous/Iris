#pragma once

#include "Utilities.h"

#include <JuceHeader.h>

struct IRSlot {
    juce::File file {};
    juce::AudioBuffer<float> buffer {};
    bool occupied = false;
    bool active = true;

    // Self-swap
    float swapMin = 0.0f, swapMax = 0.0f; // seconds
    float swapCountdown = 0.0f;

    bool swapEnabled() const { return validateSwapInterval(swapMin, swapMax); }
    void resetCountdown(juce::Random& rng) { swapCountdown = juce::jmap(rng.nextFloat(), swapMin, swapMax); }
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

    void setSwapInterval(int irIndex, float minTime, float maxTime);
    void advanceSwapTimers(float dt);

    IRChanges consumeIRChanges();
    std::atomic<bool>& getDirectoryChanged();

    const IRSlot& getIRSlot(int index) const;
    const std::vector<IRDirectory> getIRDirectories() const;
};