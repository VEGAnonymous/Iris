#pragma once

#include "Core/Control/IRDefines.h"
#include "Core/Control/State/Envelope.h"
#include "concurrentqueue.h"

#include <JuceHeader.h>

class IRManager {
private:
    juce::ApplicationProperties* applicationProperties;
    juce::AudioFormatManager formatManager;

    // IR management
    std::array<IRSlot, MAX_IR_COUNT> irSlots;

    std::vector<IRDirectory> irDirectories;
    std::vector<IRDirectoryFiles> irDirectoryFiles;

    juce::Random irRNG;
    IRSamplingMode rngMode = IRSamplingMode::UNIFORM_ACROSS_DIRECTORIES;

    std::unique_ptr<juce::FileChooser> irFileChooser;
    std::unique_ptr<juce::FileChooser> irDirectoryChooser;

    void saveDirectories();
    void loadDirectories();

    // Utilities
    void IRManager::computeEnvelope(IRSlot& slot);

    // Concurrency
    juce::SpinLock irLock;
    std::atomic<bool> directoryChanged{ false };
    std::atomic<bool> fftBusy{ false };

    moodycamel::ConcurrentQueue<IRCommand> irCommandQueue; // Forward requests
    moodycamel::ConcurrentQueue<IRResult> irResultQueue; // Forward worker output
    moodycamel::ConcurrentQueue<IRUpdate> irUpdateQueue; // Forward updates

public:
    juce::ThreadPool irThreadPool {4};

    IRManager(juce::ApplicationProperties* p);
    ~IRManager();

    void prepare();
    void enqueueCommand(IRCommand cmd);

    // Commands
    void collectIRs();

    void chooseIR(int irIndex);
    void chooseIRDirectory();

    void addIRDirectory(juce::File dir);
    void removeIRDirectory(int index);
    void setIRDirectoryActive(int index, bool nState);

    juce::AudioBuffer<float> readIR(juce::File file);
    void setIR(int irIndex, juce::File irFile, juce::AudioBuffer<float>& irBuffer);

    juce::File sampleRandomIR();
    juce::File sampleRandomIR(IRSamplingMode mode);

    void clearIR(int irIndex);
    void clearIRs();

    void setIRActive(int irIndex, bool nState);

    bool setWindow(int irIndex, float start, float length);
    void setEnvelope(int irIndex, EnvelopeType type, float attack, float release);

    juce::AudioBuffer<float> applyWindow(int irIndex) const;

    void setSwapInterval(int irIndex, float minTime, float maxTime);
    void setSwapActive(int irIndex, bool nActive);
    void advanceSwapTimers(float dt);

    void setSamplingMode(IRSamplingMode nMode);

    moodycamel::ConcurrentQueue<IRCommand>* getCommandQueue();
    moodycamel::ConcurrentQueue<IRResult>* getResultQueue();
    moodycamel::ConcurrentQueue<IRUpdate>* getUpdateQueue();

    const IRSlot& getIRSlotInternal(int index) const;
    const IRSlotLite getIRSlot(int index) const;
    
    const std::vector<IRDirectory> getIRDirectories() const;

    std::atomic<bool>& getDirectoryChanged();
    std::atomic<bool>& getFFTBusy();

    juce::AudioFormatManager* getFormatManager();
};