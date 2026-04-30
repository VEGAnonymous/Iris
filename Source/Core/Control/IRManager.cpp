#include "Core/Control/IRManager.h"

/* PRIVATE */

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
        if (dir.isDirectory()) irDirectories.push_back({ dir, active });
    }
    collectIRs();
}

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

void IRManager::prepare() {
    irDirectories.clear();
    loadDirectories();

    for (int i = 0; i < irSlots.size(); ++i) {
        computeEnvelope(irSlots[i]);
        irSlots[i].autoSwap.callback = [&, i]() {
            loadRandomIR(i, rngMode);
            irSlots[i].autoSwap.resetCountdown(irRNG);
        };
    }

    collectIRs();
}

void IRManager::collectIRs() {
    irDirectoryFiles.clear();
    for (const auto& dir : irDirectories) {
        if (!dir.active || !dir.irDirectory.isDirectory()) continue;

        IRDirectoryFiles directoryFiles;
        directoryFiles.dir = dir;
        directoryFiles.files = dir.irDirectory.findChildFiles(juce::File::findFiles, true, formatManager.getWildcardForAllFormats());

        if (!directoryFiles.files.isEmpty()) irDirectoryFiles.push_back(std::move(directoryFiles));
    }
}

void IRManager::chooseIR(int irIndex) {
    irFileChooser = std::make_unique<juce::FileChooser>(
        "Load an impulse response...", juce::File{}, formatManager.getWildcardForAllFormats());
    irFileChooser->launchAsync(juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
        [this, irIndex](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;
            loadIR(irIndex, fileChooser.getResult());
        });
}

void IRManager::chooseIRDirectory() {
    irDirectoryChooser = std::make_unique<juce::FileChooser>(
        "Choose IR directories...", juce::File{});
    irDirectoryChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories | juce::FileBrowserComponent::canSelectMultipleItems,
        [this](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;

            auto dirs = fileChooser.getResults();
            for (juce::File& dir : dirs) {
                if (dir.isRoot() || !dir.hasReadAccess()) {
                    // TODO: Apply look and feel to alert window
                    // juce::AlertWindow whoops("Oops...My bad!", "I just don't know what went wrong!", juce::MessageBoxIconType::WarningIcon);
                    // whoops.showMessageBoxAsync(juce::MessageBoxIconType::WarningIcon, "Oops...My bad!", "I just don't know what went wrong!", "Muffin?");
                    return;
                }

                for (auto& irDir : irDirectories) {
                    if (dir == irDir.irDirectory) return;
                    if (dir.isAChildOf(irDir.irDirectory)) return;
                }

                addIRDirectory(dir);

                for (int i = static_cast<int>(irDirectories.size()) - 1; i >= 0; --i) // Iterate backwards since removal shifts indices
                    if (irDirectories[i].irDirectory.isAChildOf(dir)) removeIRDirectory(i);
            }
        });
}

void IRManager::addIRDirectory(juce::File dir) {
    if (dir.isDirectory()) {
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

bool IRManager::loadIR(int irIndex, juce::File irFile) {
    if (!validateIRIndex(irIndex)) return false;

    DBG("Loading IR file into slot " << irIndex);

    // Create reader for file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
    if (!reader) return false;

    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = std::min(static_cast<int>(reader->lengthInSamples), MAX_IR_FILE_SAMPLES);

    // Read IR buffer, update state
    auto& slot = irSlots[irIndex];
    slot.buffer.setSize(numChannels, numSamples);
    if (reader->read(&slot.buffer, 0, numSamples, 0, true, true)) {
        slot.file = irFile; // File validated
        slot.occupied = true;
        // slot.active = true;

        // Reset window
        slot.window.start = 0.0f;
        slot.window.length = juce::jlimit(0.0f, 1.0f, static_cast<float>(MAX_IR_SAMPLES) / static_cast<float>(numSamples));
        // slot.window.envelope.type = EnvelopeType::NONE;

        irChanges.irsSet.push_back(irIndex); // Request update
        irChanges.irsActiveStateSet.push_back(irIndex);
        return true;
    }
    return false;
}

bool IRManager::loadRandomIR(int irIndex) { return loadRandomIR(irIndex, rngMode); }
bool IRManager::loadRandomIR(int irIndex, IRSamplingMode mode) {
    if (irDirectoryFiles.empty() || !validateIRIndex(irIndex)) return false;

    switch (mode) {
        case IRSamplingMode::UNIFORM_ACROSS_DIRECTORIES: {
            const int directoryIndex = irRNG.nextInt(static_cast<int>(irDirectoryFiles.size()));
            const auto& dir = irDirectoryFiles[directoryIndex];
            if (dir.files.isEmpty()) return false;

            const int fileIndex = irRNG.nextInt(dir.files.size());
            juce::File randomIR = dir.files[fileIndex];
            return loadIR(irIndex, randomIR);
        }
        default:
        case IRSamplingMode::UNIFORM_ACROSS_ALL_FILES: {
            int totalFiles = 0;
            for (const auto& dir : irDirectoryFiles) totalFiles += dir.files.size();
            if (totalFiles == 0) return false;

            int fileIndex = irRNG.nextInt(totalFiles);
            for (const auto& dir : irDirectoryFiles) {
                if (fileIndex < dir.files.size())
                    return loadIR(irIndex, dir.files[fileIndex]);
                fileIndex -= dir.files.size();
            }
            return false;
        }
    }
}

bool IRManager::loadRandomIRs() { return loadRandomIRs(rngMode); }
bool IRManager::loadRandomIRs(IRSamplingMode mode) {
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex)
        if (!loadRandomIR(irIndex, mode)) return false;
    return true;
}

void IRManager::clearIR(int irIndex) {
    if (!validateIRIndex(irIndex)) return;
    DBG("Clearing IR slot " << irIndex);
    auto& slot = irSlots[irIndex];
    slot.file = juce::File{};
    slot.buffer.setSize(0, 0);
    slot.occupied = false;

    irChanges.irsCleared.push_back(irIndex); // Request update
}

void IRManager::clearIRs() {
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex) clearIR(irIndex);
}

void IRManager::setIRActive(int irIndex, bool nState) {
    if (!validateIRIndex(irIndex)) return;
    auto& slot = irSlots[irIndex];
    slot.active = nState;
    irChanges.irsActiveStateSet.push_back(irIndex); // Request update
}

bool IRManager::setWindow(int irIndex, float start, float length) {
    if (!validateIRIndex(irIndex)) return false;
    auto& slot = irSlots[irIndex];
    if (slot.buffer.getNumSamples() == 0) return false;

    slot.window.start = juce::jlimit(0.0f, 1.0f, start);
    slot.window.length = juce::jlimit(0.0f, 1.0f - slot.window.start, length);

    computeEnvelope(slot);
    irChanges.irsSet.push_back(irIndex); // Request update
    return true;
}

void IRManager::setEnvelope(int irIndex, EnvelopeType type, float attack, float release) {
    if (!validateIRIndex(irIndex)) return;
    auto& slot = irSlots[irIndex];

    slot.window.envelope.type = type;
    slot.window.envelope.attack = attack; // Validate these elsewhere
    slot.window.envelope.release = release;

    computeEnvelope(slot);
    irChanges.irsSet.push_back(irIndex); // Request update
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

IRChanges IRManager::consumeIRChanges() {
    IRChanges changes = std::move(irChanges);
    irChanges = {};
    return changes;
}

void IRManager::setRandomMode(IRManager::IRSamplingMode nMode) { rngMode = nMode; }

std::atomic<bool>& IRManager::getDirectoryChanged() { return directoryChanged; }

const IRSlot& IRManager::getIRSlot(int index) const {
    jassert(validateIRIndex(index));
    return irSlots[index];
}

const std::vector<IRDirectory> IRManager::getIRDirectories() const { return irDirectories; };

juce::AudioFormatManager* IRManager::getFormatManager() { return &formatManager; }