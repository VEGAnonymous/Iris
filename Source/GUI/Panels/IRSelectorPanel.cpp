#include "Core/Control/State/GUIState.h"
#include "GUI/Panels/IRSelectorPanel.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void IRSelectorPanel::prepare() {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        irSlotButtons[i] = std::make_unique<IRSlotButton>(i, animatorUpdater, audioProcessor.guiState);

        auto* slotButton = irSlotButtons[i].get();
        slotButton->setRadioGroupId(1);
        slotButton->setClickingTogglesState(true);

        slotButton->onActiveToggle = [this, i](bool active) {
            IRCommand cmd = { IRCommand::IR_SET_ACTIVE_STATE };
            cmd.irIndex = i;
            cmd.irActiveState = active;
            audioProcessor.getIRManager()->enqueueCommand(cmd);
            audioProcessor.guiState.updateField.store(true, std::memory_order_release);
            };

        auto switchSelectedIR = [this, i]() {
            int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
            if (i != selectedIR) {
                audioProcessor.apvts.state.setProperty(PropertyID::selectedIR, i, nullptr);
                audioProcessor.guiState.selectedIRChanged.store(true, std::memory_order_relaxed);
                audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
            }
        };

        slotButton->onClick = switchSelectedIR;

        const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
        slotButton->setOccupied(slot.occupied);
        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.irWaveformLock);
            const auto& waveform = audioProcessor.guiState.irWaveforms[i];
            if (slot.occupied) {
                auto* waveformComponent = slotButton->getWaveformComponent();
                waveformComponent->setWaveform(waveform.get(), audioProcessor.getSampleRate());
                waveformComponent->setGain(slot.gain);
            }
        }
        addAndMakeVisible(*slotButton);

        // Drag and drop callbacks
        slotButton->dragHandler.onFileDropped = [this, i, switchSelectedIR](const juce::File& file) {
            switchSelectedIR();
            IRCommand cmd = { IRCommand::IR_LOAD, i };
            cmd.irFile = file;
            audioProcessor.getIRManager()->enqueueCommand(cmd);
        };

        slotButton->dragHandler.fileFilter = [this](const juce::File& file) {
            juce::String extension = file.getFileExtension();
            juce::StringArray wildcards = juce::StringArray::fromTokens(audioProcessor.getIRManager()->getFormatManager()->getWildcardForAllFormats(), ";", "");

            bool matched = false;
            for (const auto& pattern : wildcards) {
                if (extension.matchesWildcard(pattern, true)) {
                    matched = true;
                    break;
                }
            }
            return matched;
        };

        slotButton->dragHandler.onHoverChanged = [this, slotButton](bool hovering) {
            slotButton->dragHover.setValue(hovering ? 1.0f : 0.0f);
            slotButton->setMouseCursor(hovering ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::NormalCursor);
        };
    }

    int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    if (validateIRIndex(selectedIR))
        irSlotButtons[selectedIR]->setToggleState(true, juce::dontSendNotification);
}

/* PUBLIC */

IRSelectorPanel::IRSelectorPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater)
    : audioProcessor(processor), animatorUpdater(updater) {
    prepare();
}

void IRSelectorPanel::resized() {
    juce::Grid irGrid; // 2x4
    irGrid.templateRows = { juce::Grid::Fr(1), juce::Grid::Fr(1) };
    irGrid.templateColumns = { juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1) };

    for (auto& slot : irSlotButtons) irGrid.items.add(juce::GridItem(*slot));
    irGrid.performLayout(getLocalBounds());
}

void IRSelectorPanel::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::section);
    g.fillRect(getLocalBounds());
}

void IRSelectorPanel::setIndicatorStyle() {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        irSlotButtons[i]->setIndicatorStyle(
            audioProcessor.apvts.state.getProperty(PropertyID::fieldIndicatorStyle, FieldIndicatorStyle::Mareful)
        );
    }
}

IRSlotButton* IRSelectorPanel::getIRSlotButton(int irIndex) {
    jassert(validateIRIndex(irIndex));
    return irSlotButtons[irIndex].get();
}