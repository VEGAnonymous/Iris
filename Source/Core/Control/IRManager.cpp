#include "Core/Utilities.h"
#include "Core/Control/IRManager.h"
#include "GUI/GUIUtilities.h"
#include "GUI/API/MareAlert.h"

/* PRIVATE */

bool IRManager::validateIRDirectory(const juce::File& dir) const {
    return (dir.isDirectory()
        && !dir.isRoot()
        && dir.hasReadAccess()
        && !dir.findChildFiles(juce::File::findFiles, true, formatManager.getWildcardForAllFormats()).isEmpty());
}

void IRManager::saveDirectories() {
    auto* properties = applicationProperties->getUserSettings();
    if (!properties) return;

    int dirCount = static_cast<int>(irDirectories.size());
    properties->setValue("irDirectoryCount", dirCount);
    for (int i = 0; i < dirCount; ++i) {
        properties->setValue("irDirectory_" + juce::String(i), irDirectories[i].irDirectory.getFullPathName());
        properties->setValue("irDirectoryActive_" + juce::String(i), irDirectories[i].active);
    }
    properties->saveIfNeeded();
}

void IRManager::loadDirectories() {
    auto* properties = applicationProperties->getUserSettings();
    if (!properties) return;

    int dirCount = properties->getIntValue("irDirectoryCount", 0);
    for (int i = 0; i < dirCount; ++i) {
        juce::File dir(properties->getValue("irDirectory_" + juce::String(i)));
        bool active = properties->getBoolValue("irDirectoryActive_" + juce::String(i), true);
        if (validateIRDirectory(dir)) irDirectories.push_back({ dir, active });
    }
    collectIRs();
}

// Utilities

void IRManager::computeEnvelope(IRSlot& slot) {
    if (slot.buffer.getNumSamples() == 0) return;
    slot.window.envelope.length = juce::jlimit(1,
        std::min(MAX_IR_SAMPLES, slot.buffer.getNumSamples()), 
        static_cast<int>(slot.window.length * slot.buffer.getNumSamples()));

    slot.window.envelope.computeEnvelope();
}

/* PUBLIC */

IRManager::IRManager(juce::ApplicationProperties* p) : applicationProperties(p) {
    formatManager.registerBasicFormats(); // .wav, .aiff, .flac, .opus, .mp3
    
    irRNG = juce::Random();
    irRNG.setSeedRandomly();
}

IRManager::~IRManager() { 
    irThreadPool.removeAllJobs(false, 1000);
}

void IRManager::prepare() {
    irDirectories.clear();
    loadDirectories();

    for (int i = 0; i < irSlots.size(); ++i) {
        computeEnvelope(irSlots[i]);
        irSlots[i].autoSwap.callback = [&, i]() {
            enqueueCommand(IRCommand{ IRCommand::IR_LOAD_RANDOM, i });
            irSlots[i].autoSwap.resetCountdown(irRNG);
        };
    }

    collectIRs();
}

void IRManager::enqueueCommand(IRCommand cmd) { irCommandQueue.enqueue(cmd); }

void IRManager::collectIRs() {
    collectPending.store(true, std::memory_order_release);
    if (busyCollecting.exchange(true, std::memory_order_acq_rel)) return;
    irThreadPool.addJob([this]() {
        do {
            collectPending.store(false, std::memory_order_release);
            auto newFiles = std::make_shared<std::vector<IRDirectoryFiles>>();
            for (const auto& dir : irDirectories) {
                if (!dir.active || !dir.irDirectory.isDirectory()) continue;

                IRDirectoryFiles directoryFiles;
                directoryFiles.dir = dir;
                directoryFiles.files = dir.irDirectory.findChildFiles(juce::File::findFiles, true, formatManager.getWildcardForAllFormats());

                if (!directoryFiles.files.isEmpty()) newFiles->push_back(std::move(directoryFiles));
            }
            std::atomic_store(&irDirectoryFiles, newFiles);
            directoryChanged.store(true, std::memory_order_release);
        } while (collectPending.load(std::memory_order_acquire)); // Don't drop changes

        DBG("Finished collecting IRs");
        busyCollecting.store(false, std::memory_order_release);
    });
}

void IRManager::chooseIR(int irIndex) { // Only to be called from the message thread!
    irFileChooser = std::make_unique<juce::FileChooser>(
        "Load an impulse response", juce::File{}, formatManager.getWildcardForAllFormats());
    const juce::MessageManagerLock mmLock;
    irFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, irIndex](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;
            IRCommand cmd = IRCommand{ IRCommand::IR_LOAD, irIndex };
            cmd.irFile = fileChooser.getResult();
            enqueueCommand(cmd);
        }
    );
}

void IRManager::chooseIRDirectory() { // Only to be called from the message thread!
    irDirectoryChooser = std::make_unique<juce::FileChooser>(
        "Choose IR directories", juce::File{});
    const juce::MessageManagerLock mmLock;
    irDirectoryChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::canSelectMultipleItems,
        [this](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;

            auto dirs = fileChooser.getResults();
            juce::String invalidDirectories = "";
            int invalidCount = 0;

            for (juce::File& dir : dirs) {
                if (!validateIRDirectory(dir) && invalidCount < 10) {
                    invalidCount++;
                    if (invalidCount < 10) invalidDirectories.append(formatPath(dir.getFullPathName(), 40, Ellipsis::MIDDLE) + "\n", 42);
                    else invalidDirectories.append("...", 3);
                    continue;
                }

                for (auto& irDir : irDirectories) {
                    if (dir == irDir.irDirectory) return;
                    if (dir.isAChildOf(irDir.irDirectory)) return;
                }

                addIRDirectory(dir);

                for (int i = static_cast<int>(irDirectories.size()) - 1; i >= 0; --i) // Iterate backwards since removal shifts indices
                    if (irDirectories[i].irDirectory.isAChildOf(dir)) removeIRDirectory(i);
            }

            if (onAlert && invalidDirectories.isNotEmpty()) {
                onAlert(
                    "Oops...My bad!",
                    "I just don't know what went wrong!",
                    invalidDirectories,
                    "Muffin?"
                );
            }
        }
    );
}

void IRManager::addIRDirectory(juce::File dir) {
    if (validateIRDirectory(dir)) {
        irDirectories.push_back({ dir, true });
        collectIRs();
        saveDirectories();
        directoryChanged.store(true, std::memory_order_release);
    }
}
void IRManager::removeIRDirectory(int index) {
    if (index >= 0 && index < static_cast<int>(irDirectories.size())) {
        irDirectories.erase(irDirectories.begin() + index);
        collectIRs();
        saveDirectories();
        directoryChanged.store(true, std::memory_order_release);
    }
}
void IRManager::setIRDirectoryActive(int index, bool nState) {
    if (index >= 0 && index < static_cast<int>(irDirectories.size())) {
        irDirectories[index].active = nState;
        collectIRs();
        saveDirectories();
        directoryChanged.store(true, std::memory_order_release);
    }
}

juce::AudioBuffer<float> IRManager::readIR(juce::File irFile) {
    // Create reader for file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
    if (!reader) return juce::AudioBuffer<float> {};

    // Read IR buffer
    const int numChannels = reader->numChannels;
    const int numSamples = std::min(static_cast<int>(reader->lengthInSamples), MAX_IR_FILE_SAMPLES);

    juce::AudioBuffer<float> buffer;
    buffer.setSize(numChannels, numSamples);
    bool readSuccess = reader->read(&buffer, 0, numSamples, 0, true, true);
    jassert(readSuccess);
    return buffer;
}

void IRManager::setIR(int irIndex, juce::File irFile, juce::AudioBuffer<float>& irBuffer) {
    if (!validateIRIndex(irIndex)) return;

    const int numSamples = irBuffer.getNumSamples();

    auto& slot = irSlots[irIndex];
    slot.buffer = std::move(irBuffer);
    slot.file = irFile;
    slot.occupied = true;
    slot.window.start = 0.0f;
    slot.window.length = juce::jlimit(0.0f, 1.0f, static_cast<float>(MAX_IR_SAMPLES) / static_cast<float>(numSamples));

    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_SET, irIndex });
    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_ACTIVE_CHANGED, irIndex });
}

juce::File IRManager::sampleRandomIR() { return sampleRandomIR(rngMode); }
juce::File IRManager::sampleRandomIR(IRSamplingMode mode) {
    if ((*irDirectoryFiles).empty()) return juce::File();

    switch (mode) {
        case IRSamplingMode::UNIFORM_ACROSS_DIRECTORIES: {
            const int directoryIndex = irRNG.nextInt(static_cast<int>((*irDirectoryFiles).size()));
            const auto& dir = (*irDirectoryFiles)[directoryIndex];
            if (dir.files.isEmpty()) return juce::File();

            int fileIndex = -1;
            {
                juce::SpinLock::ScopedLockType lock(irLock);
                fileIndex = irRNG.nextInt(dir.files.size());
                jassert(fileIndex >= 0);
            }
            return dir.files[fileIndex];
        }
        default:
        case IRSamplingMode::UNIFORM_ACROSS_ALL_FILES: {
            int totalFiles = 0;
            for (const auto& dir : *irDirectoryFiles) totalFiles += dir.files.size();
            if (totalFiles == 0) return juce::File();

            int fileIndex = -1;
            {
                juce::SpinLock::ScopedLockType lock(irLock);
                fileIndex = irRNG.nextInt(totalFiles);
                jassert(fileIndex >= 0);
            }
            for (const auto& dir : *irDirectoryFiles) {
                if (fileIndex < dir.files.size()) return dir.files[fileIndex];
                fileIndex -= dir.files.size();
            }
            return juce::File();
        }
    }
}

void IRManager::clearIR(int irIndex) {
    if (!validateIRIndex(irIndex)) return;
    // DBG("Clearing IR slot " << irIndex);
    auto& slot = irSlots[irIndex];
    slot.file = juce::File{};
    slot.buffer.setSize(0, 0);
    slot.occupied = false;

    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_CLEAR, irIndex });
}

void IRManager::clearIRs() {
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex) clearIR(irIndex);
}

void IRManager::setIRActive(int irIndex, bool nState) {
    if (!validateIRIndex(irIndex)) return;
    auto& slot = irSlots[irIndex];
    slot.active = nState;
    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_ACTIVE_CHANGED, irIndex });
}

bool IRManager::setWindow(int irIndex, float start, float length) {
    if (!validateIRIndex(irIndex)) return false;
    auto& slot = irSlots[irIndex];
    if (slot.buffer.getNumSamples() == 0) return false;

    slot.window.start = juce::jlimit(0.0f, 1.0f, start);
    slot.window.length = juce::jlimit(0.0f, 1.0f - slot.window.start, length);

    computeEnvelope(slot);
    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_SET, irIndex });
    return true;
}

void IRManager::setEnvelope(int irIndex, EnvelopeType type, float attack, float release) {
    if (!validateIRIndex(irIndex)) return;
    auto& slot = irSlots[irIndex];

    slot.window.envelope.type = type;
    slot.window.envelope.attack = attack; // Validate these elsewhere
    slot.window.envelope.release = release;

    computeEnvelope(slot);
    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_SET, irIndex });
}

juce::AudioBuffer<float> IRManager::applyWindow(int irIndex) const {
    const auto& slot = irSlots[irIndex];
    const int bufferSamples = slot.buffer.getNumSamples();
    if (bufferSamples == 0) return {};

    const int numChannels = std::min(slot.buffer.getNumChannels(), N_CHANNELS);
    const int startSamples = static_cast<int>(slot.window.start * bufferSamples);
    const int lengthSamples = juce::jlimit(1, 
        std::min(MAX_IR_SAMPLES, bufferSamples - startSamples),
        static_cast<int>(slot.window.length * bufferSamples));

    // Copy windowed samples and envelope
    // DBG("Applying envelope: " << irIndex);
    juce::AudioBuffer<float> out(numChannels, lengthSamples);
    for (int channel = 0; channel < numChannels; ++channel) {
        out.copyFrom(channel, 0, slot.buffer, channel, startSamples, lengthSamples);
        if (static_cast<int>(slot.window.envelope.curve.size()) >= lengthSamples)
            juce::FloatVectorOperations::multiply(out.getWritePointer(channel), slot.window.envelope.curve.data(), lengthSamples);
    }
    
    return out;
}

void IRManager::setSwapInterval(int irIndex, float nMin, float nMax) {
    auto& slotSwap = irSlots[irIndex].autoSwap;
    if (nMin != slotSwap.minTime || nMax != slotSwap.maxTime) {
        slotSwap.minTime = nMin;
        slotSwap.maxTime = nMax;
        slotSwap.resetCountdown(irRNG);
    }
}

void IRManager::setSwapActive(int irIndex, bool nActive) {
    auto& slotSwap = irSlots[irIndex].autoSwap;
    slotSwap.active = nActive;
    if (nActive) slotSwap.resetCountdown(irRNG);
}

void IRManager::advanceSwapTimers(float dt) {
    for (int i = 0; i < MAX_IR_COUNT; ++i)
        irSlots[i].autoSwap.advanceTimer(dt);
}

void IRManager::setSamplingMode(IRSamplingMode nMode) { rngMode = nMode; }

std::atomic<bool>& IRManager::getDirectoryChanged() { return directoryChanged; }
std::atomic<bool>& IRManager::getBusyLoading() { return busyLoading; }
std::atomic<bool>& IRManager::getBusyCollecting() { return busyCollecting; }

moodycamel::ConcurrentQueue<IRCommand>* IRManager::getCommandQueue() { return &irCommandQueue; };
moodycamel::ConcurrentQueue<IRResult>* IRManager::getResultQueue() { return &irResultQueue; };
moodycamel::ConcurrentQueue<IRUpdate>* IRManager::getUpdateQueue() { return &irUpdateQueue; };

const IRSlot& IRManager::getIRSlotInternal(int index) const {
    jassert(validateIRIndex(index));
    return irSlots[index];
}

const IRSlotLite IRManager::getIRSlot(int index) const {
    jassert(validateIRIndex(index));
    juce::SpinLock::ScopedLockType lock(irLock);

    const auto& slot = irSlots[index];
    return IRSlotLite{
        slot.file,
        slot.occupied,
        slot.active,
        slot.window,
        slot.getMaxWindowLength(),
        slot.buffer.getNumSamples()
    };
}

const std::vector<IRDirectory> IRManager::getIRDirectories() const { return irDirectories; };

juce::AudioFormatManager* IRManager::getFormatManager() { return &formatManager; }