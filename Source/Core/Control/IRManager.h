#pragma once

#include "Core/Control/IRDefines.h"
#include "Core/Control/State/Envelope.h"
#include "GUI/Theme/LookAndFeel/MareverbLookAndFeel.h"
#include "concurrentqueue.h"

#include <JuceHeader.h>

class IRManager {
private:
    juce::ApplicationProperties* applicationProperties;
    juce::AudioFormatManager formatManager;

    MareverbLookAndFeel mainLookAndFeel;

    // IR management
    std::array<IRSlot, MAX_IR_COUNT> irSlots;

    std::vector<IRDirectory> irDirectories;
    std::shared_ptr<std::vector<IRDirectoryFiles>> irDirectoryFiles { std::make_shared<std::vector<IRDirectoryFiles>>() };

    juce::Random irRNG;
    IRSamplingMode samplingMode = IRSamplingMode::UNIFORM_ACROSS_DIRECTORIES;
    juce::StringArray fileFilter;
    juce::StringArray directoryFilter;

    std::unique_ptr<juce::FileChooser> irFileChooser;
    std::unique_ptr<juce::FileChooser> irDirectoryChooser;

    // Utilities
    inline bool validateIRDirectory(const juce::File& dir) const;

    static juce::StringArray parseFilter(const juce::String& filter);
    static bool matchesKeyword(const juce::String& text, const juce::StringArray keywords);

    void computeEnvelope(IRSlot& slot);

    // Concurrency
    juce::SpinLock irLock;
    juce::SpinLock dirLock;
    std::atomic<bool> directoryChanged{ false };
    std::atomic<bool> irLoaded { false };
    std::atomic<bool> busyLoading { false };
    std::atomic<bool> busyCollecting { false };
    std::atomic<bool> collectPending { false };

    moodycamel::ConcurrentQueue<IRCommand> irCommandQueue; // Forward requests
    moodycamel::ConcurrentQueue<IRResult> irResultQueue; // Forward worker output
    moodycamel::ConcurrentQueue<IRUpdate> irUpdateQueue; // Forward updates

public:
    juce::ThreadPool irThreadPool { juce::jmax(1, juce::SystemStats::getNumCpus() / 3) };

    std::function<void(juce::String title, juce::String message, juce::String details, juce::String buttonText)> onAlert;

    IRManager(juce::ApplicationProperties* p);
    ~IRManager();

    void prepare();
    void enqueueCommand(IRCommand cmd);

    void saveDirectories();
    void loadDirectories();

    void collectIRs();
    void collectIRs(const juce::StringArray fileFilterKeywords, const juce::StringArray directoryFilterKeywords);

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

    void setIRGainDB(int irIndex, float gainDB);

    bool setWindow(int irIndex, float start, float length);
    void setEnvelope(int irIndex, EnvelopeType type, float attack, float release);

    juce::AudioBuffer<float> applyWindow(int irIndex) const;

    void setSwapInterval(int irIndex, float minTime, float maxTime);
    void setSwapActive(int irIndex, bool nActive);
    void advanceSwapTimers(float dt);

    void setFileFilter(juce::String fileFilter = "");
    void setDirectoryFilter(juce::String directoryFilter = "");

    void setSamplingMode(IRSamplingMode nMode);

    moodycamel::ConcurrentQueue<IRCommand>* getCommandQueue();
    moodycamel::ConcurrentQueue<IRResult>* getResultQueue();
    moodycamel::ConcurrentQueue<IRUpdate>* getUpdateQueue();

    const IRSlot& getIRSlotInternal(int index) const;
    const IRSlotLite getIRSlot(int index) const;
    
    const std::vector<IRDirectory> getIRDirectories() const;
    std::shared_ptr<std::vector<IRDirectoryFiles>> getIRDirectoryFiles() const;
    int getIRFileCount();

    std::atomic<bool>& getDirectoryChanged();
    std::atomic<bool>& getIRLoaded();
    std::atomic<bool>& getBusyLoading();
    std::atomic<bool>& getBusyCollecting();

    juce::AudioFormatManager* getFormatManager();
};