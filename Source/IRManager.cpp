#include "IRManager.h"

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

void IRManager::collectIRs() {
    irFiles.clear();
    for (const auto& dir : irDirectories) {
        if (!dir.active || !dir.irDirectory.isDirectory()) continue;
        irFiles.addArray(dir.irDirectory.findChildFiles(juce::File::findFiles, true, formatManager.getWildcardForAllFormats()));
    }
}

/* PUBLIC */

IRManager::IRManager(juce::ApplicationProperties* p) : applicationProperties(p) {
    formatManager.registerBasicFormats(); // .wav, .aiff, .flac, .opus, .mp3
    
    irRNG = juce::Random();
    irRNG.setSeedRandomly();
}

void IRManager::prepare() {
    irDirectories.clear();

    // HACK: Should store factory IRs in common directory
    juce::File srcDirectory = juce::File::getSpecialLocation(juce::File::currentApplicationFile);
    while (srcDirectory.getFileName() != "Debug" && srcDirectory.getParentDirectory() != srcDirectory)
        srcDirectory = srcDirectory.getParentDirectory();
    irDirectories.push_back({ srcDirectory.getChildFile("IR Samples"), true });

    loadDirectories();

    bool loadedIRs = loadRandomIRs();
    jassert(loadedIRs);
    // clearIRs();
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
        "Choose a directory...", juce::File{});
    irDirectoryChooser->launchAsync(
        juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectDirectories,
        [this](const juce::FileChooser& fileChooser) {
            if (fileChooser.getResults().isEmpty()) return;
            addIRDirectory(fileChooser.getResult());
        });
}

void IRManager::addIRDirectory(juce::File dir) {
    if (dir.isDirectory()) {
        irDirectories.push_back({ dir, true });
        collectIRs();
        saveDirectories();
    }
}
void IRManager::removeIRDirectory(int index) {
    if (index >= 1 && index < static_cast<int>(irDirectories.size())) { // index 0 = factory
        irDirectories.erase(irDirectories.begin() + index);
        collectIRs();
        saveDirectories();
    }
}
void IRManager::activateIRDirectory(int index, bool nState) {
    if (index >= 0 && index < static_cast<int>(irDirectories.size())) {
        irDirectories[index].active = nState;
        collectIRs();
        saveDirectories();
    }
}

bool IRManager::loadIR(int irIndex, juce::File irFile) {
    DBG("Loading IR file into slot " << irIndex);
    if (!validateIRIndex(irIndex)) return false;

    // Create reader for file
    std::unique_ptr<juce::AudioFormatReader> reader(formatManager.createReaderFor(irFile));
    if (!reader) return false;

    const int numChannels = static_cast<int>(reader->numChannels);
    const int numSamples = static_cast<int>(reader->lengthInSamples);

    // Read IR buffer, update state
    auto& slot = irSlots[irIndex];
    slot.buffer.setSize(numChannels, numSamples);
    if (reader->read(&slot.buffer, 0, numSamples, 0, true, true)) {
        slot.file = irFile; // File validated
        slot.occupied = true;

        irChanges.irsChanged.push_back(irIndex); // Request update
        return true;
    }
    return false;
}

bool IRManager::loadRandomIR(int irIndex) {
    if (irFiles.isEmpty() || !validateIRIndex(irIndex)) return false;

    int idx = irRNG.nextInt(irFiles.size());
    juce::File randomIR = irFiles[idx];
    DBG("Generated random index: " << idx);
    return loadIR(irIndex, randomIR);
}

bool IRManager::loadRandomIRs() {
    if (irFiles.isEmpty()) return false;
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex)
        if (!loadRandomIR(irIndex)) return false;
    return true;
}

void IRManager::clearIR(int irIndex) {
    if (validateIRIndex(irIndex)) {
        DBG("Clearing IR slot " << irIndex);
        auto& slot = irSlots[irIndex];
        slot.file = juce::File{};
        slot.buffer.setSize(0, 0);
        slot.occupied = false;

        irChanges.irsCleared.push_back(irIndex); // Request update
    }
}

void IRManager::clearIRs() {
    for (int irIndex = 0; irIndex < MAX_IR_COUNT; ++irIndex) clearIR(irIndex);
}

void IRManager::setSwapInterval(int irIndex, float nMin, float nMax) {
    if (validateSwapInterval(nMin, nMax)) {
        auto& slot = irSlots[irIndex];
        if (nMin != slot.swapMin || nMax != slot.swapMax) {
            slot.swapMin = nMin;
            slot.swapMax = nMax;
            if (slot.swapEnabled()) slot.resetCountdown(irRNG);
            else slot.swapCountdown = 0.0f;
        }
    }
}

void IRManager::advanceSwapTimers(float dt) {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        auto& slot = irSlots[i];
        if (!slot.swapEnabled()) continue;

        slot.swapCountdown -= dt;
        if (slot.swapCountdown <= 0.0f) {
            loadRandomIR(i);
            slot.resetCountdown(irRNG);
        }
    }
}

IRChanges IRManager::consumeIRChanges() {
    IRChanges changes = std::move(irChanges);
    irChanges = {};
    return changes;
}

const IRSlot& IRManager::getIRSlot(int index) const {
    jassert(validateIRIndex(index));
    return irSlots[index];
}