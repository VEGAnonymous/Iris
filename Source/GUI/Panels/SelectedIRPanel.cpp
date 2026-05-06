#include "GUI/GUIUtilities.h"
#include "GUI/Panels/SelectedIRPanel.h"

/* PRIVATE */

void SelectedIRPanel::prepare() {
    addAndMakeVisible(irHeaderComponent);
    addAndMakeVisible(irDisplayComponent);
    addAndMakeVisible(irControlsComponent);
}

/* PUBLIC */

SelectedIRPanel::SelectedIRPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, ValueTooltipWindow& tooltip, juce::Component& parent)
    : audioProcessor(processor), animatorUpdater(updater),
      irHeaderComponent(processor, updater), 
      irDisplayComponent(audioProcessor, updater), 
      irControlsComponent(audioProcessor, updater, tooltip, parent) {
    prepare();
}

void SelectedIRPanel::resized() {
    Bounds bounds = getLocalBounds();
    irHeaderComponent.setBounds(bounds.removeFromTop(40).reduced(PANEL_INSET));
    irDisplayComponent.setBounds(bounds.removeFromTop(140).reduced(PANEL_INSET));
    irControlsComponent.setBounds(bounds.removeFromTop(100).reduced(PANEL_INSET));
}

void SelectedIRPanel::updateIRSlot(bool animate) {
    int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    if (validateIRIndex(selectedIR)) {
        // IR display
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.irWaveformLock);
            const auto& waveform = audioProcessor.guiState.irWaveforms[selectedIR];

            irHeaderComponent.setActive(slot.active, animate);
            irHeaderComponent.setSlot(selectedIR, slot);
            irHeaderComponent.updateIndicator();

            auto* waveformComponent = irDisplayComponent.getWaveformComponent();
            jassert(waveformComponent);
            if (waveformComponent) {
                waveformComponent->setWaveform(slot.occupied ? waveform.get() : nullptr, audioProcessor.getSampleRate());
                waveformComponent->setGain(slot.gain);
                waveformComponent->setColor(Theme::Colors::irSlotColours[selectedIR]);
                waveformComponent->setActive(slot.active, animate);
            }
        }

        auto* windowOverlayComponent = irDisplayComponent.getWindowOverlayComponent();
        jassert(windowOverlayComponent);
        if (windowOverlayComponent) {
            windowOverlayComponent->setMaxLength(slot.maxWindowLength);
            windowOverlayComponent->setWindow(slot.window.start, slot.window.start + slot.window.length);
        }

        // IR controls
        // Envelope
        auto* envelopeControl = irControlsComponent.getEnvelopeControl();
        jassert(envelopeControl);
        if (envelopeControl) envelopeControl->setEnvelope(slot.window.envelope);

        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            // Generic
            auto* irControl = irControlsComponent.getIRControl(i);
            jassert(irControl);
            if (irControl) {
                irControl->gainControl.setVisible(i == selectedIR);
                irControl->gainControl.control.setValue(juce::Decibels::gainToDecibels(slot.gain), juce::dontSendNotification);
            }

            // Swap controls
            auto* swapControl = irControlsComponent.getSwapControl(i);
            jassert(swapControl);
            if (swapControl) {
                swapControl->swapActiveToggle.setVisible(i == selectedIR);
                swapControl->swapRangeSlider.setVisible(i == selectedIR);

                swapControl->swapActiveToggle.control.setToggleState(slot.autoSwap.active, juce::dontSendNotification);
                swapControl->swapRangeSlider.control.setRange(
                    juce::jmap(slot.autoSwap.minTime, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX, 0.0f, 1.0f),
                    juce::jmap(slot.autoSwap.maxTime, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX, 0.0f, 1.0f)
                );
            }
        }
    }
}

void SelectedIRPanel::updateIndicatorStyle() {
    irHeaderComponent.setIndicatorStyle(
        audioProcessor.apvts.state.getProperty(PropertyID::fieldIndicatorStyle, FieldIndicatorStyle::Mareful));
}

IRDisplayComponent* SelectedIRPanel::getIRDisplayComponent() { return &irDisplayComponent; }
IRControlsComponent* SelectedIRPanel::getIRControlsComponent() { return &irControlsComponent; }