#include "Core/Utilities.h"
#include "Core/Control/IRManager.h"
#include "GUI/GUIUtilities.h"
#include "GUI/API/MareAlert.h"

/* PRIVATE */

bool IRManager::validateIRDirectory(const juce::File& dir) const {
    return (dir.isDirectory()
        && !dir.isRoot()
        && dir.hasReadAccess());
}

// Utilities

juce::StringArray IRManager::parseFilter(const juce::String& filter) {
    juce::StringArray keywords = juce::StringArray::fromTokens(filter.toLowerCase(), ";,", "");
    keywords.trim();
    keywords.removeDuplicates(true);
    keywords.removeEmptyStrings();
    return keywords;
};

bool IRManager::matchesKeyword(const juce::String& text, const juce::StringArray keywords) {
    if (keywords.isEmpty()) return true;

    const juce::String lowerText = text.toLowerCase();
    for (const auto& keyword : keywords)
        if (keyword.isNotEmpty() && lowerText.contains(keyword)) return true;
    return false;
};

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
    irThreadPool.removeAllJobs(true, 6210);
}

void IRManager::prepare() {
    irDirectories.clear();
    loadDirectories();

    for (int i = 0; i < irSlots.size(); ++i) {
        computeEnvelope(irSlots[i]);
        irSlots[i].autoSwap.callback = [&, i]() {
            DBG("IR: Swapped slot " << i);
            enqueueCommand(IRCommand{ IRCommand::IR_LOAD_RANDOM, i });
            irSlots[i].autoSwap.resetCountdown(irRNG);
        };
    }
}

void IRManager::enqueueCommand(IRCommand cmd) { irCommandQueue.enqueue(cmd); }

void IRManager::saveDirectories() {
    auto* properties = applicationProperties->getUserSettings();
    if (!properties) return;
    if (!properties->isValidFile()) return;

    int oldCount = properties->getIntValue(PropertyID::IRDirectory::irDirectoryCount);
    int newCount = static_cast<int>(irDirectories.size());
    properties->setValue(PropertyID::IRDirectory::irDirectoryCount, newCount);
    for (int i = 0; i < newCount; ++i) {
        properties->setValue(PropertyID::IRDirectory::irDirectoryPath(i), irDirectories[i].irDirectory.getFullPathName());
        properties->setValue(PropertyID::IRDirectory::irDirectoryActive(i), irDirectories[i].active);
    }

    // Cleanup leftover entries
    for (int i = newCount; i < oldCount; ++i) {
        properties->removeValue(PropertyID::IRDirectory::irDirectoryPath(i));
        properties->removeValue(PropertyID::IRDirectory::irDirectoryActive(i));
    }

    properties->saveIfNeeded();
}

void IRManager::loadDirectories() {
    auto* properties = applicationProperties->getUserSettings();
    if (!properties) return;
    if (!properties->isValidFile()) return;

    properties->reload();

    irDirectories.clear();

    int dirCount = properties->getIntValue(PropertyID::IRDirectory::irDirectoryCount, 0);
    for (int i = 0; i < dirCount; ++i) {
        juce::File dir(properties->getValue(PropertyID::IRDirectory::irDirectoryPath(i)));
        bool active = properties->getBoolValue(PropertyID::IRDirectory::irDirectoryActive(i), true);
        if (validateIRDirectory(dir)) irDirectories.push_back({ dir, active });
    }

    IRCommand cmd = { IRCommand::IR_DIRECTORY_COLLECT };
    enqueueCommand(cmd);
}

void IRManager::collectIRs() { collectIRs(fileFilter, directoryFilter); }
void IRManager::collectIRs(const juce::StringArray fileKeywords, const juce::StringArray directoryKeywords) {
    DBG("IR: Collecting IRs with file filter (" << fileKeywords.joinIntoString(", ") << "); directory filter (" << directoryKeywords.joinIntoString(", ") << ")");
    collectPending.store(true, std::memory_order_release);
    busyCollecting.store(true, std::memory_order_release);
    directoryChanged.store(true, std::memory_order_release);

    irThreadPool.addJob([this, fileKeywords, directoryKeywords]() {
        const juce::String formatWildcard = formatManager.getWildcardForAllFormats();
        do {
            collectPending.store(false, std::memory_order_release);
            auto nFiles = std::make_shared<std::vector<IRDirectoryFiles>>();

            int directoryListSize;
            {
                juce::SpinLock::ScopedLockType lock(irLock);
                directoryListSize = static_cast<int>(irDirectories.size());
            }

            DBG("IR: Running collection pass; found " << directoryListSize << " directories");

            for (int i = 0; i < directoryListSize; ++i) {
                IRDirectory dir {};
                {
                    juce::SpinLock::ScopedLockType lock(irLock);
                    if (i >= irDirectories.size()) break;
                    else dir = irDirectories[i];
                }

                if (!dir.active || !dir.irDirectory.isDirectory()) continue;

                auto irFiles = dir.irDirectory.findChildFiles(juce::File::findFiles, true, formatWildcard);

                IRDirectoryFiles directoryFiles;
                directoryFiles.dir = dir;

                for (const auto& file : irFiles) {
                    // Match directory filter by path components
                    if (!directoryKeywords.isEmpty()) {
                        const auto pathComponents = juce::StringArray::fromTokens(file.getRelativePathFrom(dir.irDirectory), "/\\", "");
                        bool matches = false;
                        for (int j = 0; j < pathComponents.size() - 1; ++j) { // size - 1 to exclude filename
                            if (matchesKeyword(pathComponents[j], directoryKeywords)) {
                                matches = true;
                                break;
                            }
                        }
                        if (!matches) continue;
                    }

                    // Match file filter
                    if (!matchesKeyword(file.getFileNameWithoutExtension(), fileKeywords)) continue;

                    directoryFiles.files.add(file);
                }

                if (!directoryFiles.files.isEmpty()) nFiles->push_back(std::move(directoryFiles));
            }

            std::atomic_store(&irDirectoryFiles, nFiles);
            directoryChanged.store(true, std::memory_order_release);

        } while (collectPending.load(std::memory_order_acquire)); // Don't drop changes

        DBG("IR: Finished collecting IRs");
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
    reader->read(&buffer, 0, numSamples, 0, true, true);
    return buffer;
}

void IRManager::setIR(int irIndex, juce::File irFile, juce::AudioBuffer<float>& irBuffer) {
    if (!validateIRIndex(irIndex)) return;

    const int numSamples = irBuffer.getNumSamples();

    auto& slot = irSlots[irIndex];
    slot.buffer = std::move(irBuffer);
    slot.file = irFile;
    slot.occupied = true;
    slot.gain = 1.0f;
    slot.window.start = 0.0f;
    slot.window.length = juce::jlimit(0.0f, 1.0f, static_cast<float>(MAX_IR_SAMPLES) / static_cast<float>(numSamples));

    // Enqueued by controller
    // irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_SET, irIndex }); 
    // irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_ACTIVE_CHANGED, irIndex });
    // irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_GAIN_CHANGED, irIndex });

    irLoaded.store(true, std::memory_order_release);
}

juce::File IRManager::sampleRandomIR() { return sampleRandomIR(samplingMode); }
juce::File IRManager::sampleRandomIR(IRSamplingMode mode) {
    if ((*irDirectoryFiles).empty()) return juce::File();

    switch (mode) {
        case IRSamplingMode::UNIFORM_ACROSS_DIRECTORIES: {
            const int directoryIndex = irRNG.nextInt(static_cast<int>((*irDirectoryFiles).size()));
            const auto& dir = (*irDirectoryFiles)[directoryIndex];
            if (dir.files.isEmpty()) return juce::File();

            int fileIndex = -1;
            {
                juce::SpinLock::ScopedLockType lock(dirLock);
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
                juce::SpinLock::ScopedLockType lock(dirLock);
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

void IRManager::setIRGainDB(int irIndex, float gainDB) {
    const float nGain = juce::Decibels::decibelsToGain(gainDB);
    if (nGain != irSlots[irIndex].gain) {
        irSlots[irIndex].gain = nGain;
        irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_GAIN_CHANGED, irIndex });
    }
}

bool IRManager::setWindow(int irIndex, float start, float length) {
    if (!validateIRIndex(irIndex)) return false;
    auto& slot = irSlots[irIndex];
    if (slot.buffer.getNumSamples() == 0) return false;

    slot.window.start = juce::jlimit(0.0f, 1.0f, start);
    slot.window.length = juce::jlimit(0.0f, 1.0f - slot.window.start, length);

    computeEnvelope(slot);
    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_SET, irIndex, false });
    return true;
}

void IRManager::setEnvelope(int irIndex, EnvelopeType type, float attack, float release) {
    if (!validateIRIndex(irIndex)) return;
    auto& slot = irSlots[irIndex];

    slot.window.envelope.type = type;
    slot.window.envelope.attack = attack; // Validate these elsewhere
    slot.window.envelope.release = release;

    // DBG("IR: Set IR " << irIndex << " envelope type = " << static_cast<int>(type) << ", attack = " << slot.window.envelope.attack << ", release = " << slot.window.envelope.release);

    computeEnvelope(slot);
    irUpdateQueue.enqueue(IRUpdate{ IRUpdate::IR_SET, irIndex, false });
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
    if (nActive != slotSwap.active) {
        slotSwap.active = nActive;
        slotSwap.resetCountdown(irRNG);
    }
}

void IRManager::advanceSwapTimers(float dt) {
    for (int i = 0; i < MAX_IR_COUNT; ++i)
        irSlots[i].autoSwap.advanceTimer(dt);
}

void IRManager::setFileFilter(juce::String nFilter) {
    fileFilter = parseFilter(nFilter);
    collectIRs();
}

void IRManager::setDirectoryFilter(juce::String nFilter) {
    directoryFilter = parseFilter(nFilter);
    collectIRs();
}

void IRManager::setSamplingMode(IRSamplingMode nMode) { samplingMode = nMode; }

std::atomic<bool>& IRManager::getDirectoryChanged() { return directoryChanged; }
std::atomic<bool>& IRManager::getIRLoaded() { return irLoaded; }
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
        slot.gain,
        slot.window,
        slot.getMaxWindowLength(),
        slot.autoSwap,
        slot.buffer.getNumSamples()
    };
}

const std::vector<IRDirectory> IRManager::getIRDirectories() const { return irDirectories; };
std::shared_ptr<std::vector<IRDirectoryFiles>> IRManager::getIRDirectoryFiles () const { return irDirectoryFiles; };

int IRManager::getIRFileCount() {
    int fileCount = 0;

    juce::SpinLock::ScopedLockType lock(dirLock);
    for (auto& directoryFiles : *irDirectoryFiles)
        fileCount += directoryFiles.files.size();

    return fileCount;
}

juce::AudioFormatManager* IRManager::getFormatManager() { return &formatManager; }