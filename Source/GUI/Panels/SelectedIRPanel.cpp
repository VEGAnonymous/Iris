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

void SelectedIRPanel::updateIRSlot(int selectedIR, bool animate) {
    if (validateIRIndex(selectedIR)) {
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.irWaveformLock);
            const auto& waveform = audioProcessor.guiState.irWaveforms[selectedIR];

            irHeaderComponent.setActive(slot.active, animate);
            irHeaderComponent.setSlot(selectedIR, slot);

            auto* waveformComponent = irDisplayComponent.getWaveformComponent();
            jassert(waveformComponent);
            if (waveformComponent) {
                waveformComponent->setNumPoints(WAVEFORM_POINTS);
                waveformComponent->setWaveform(slot.occupied ? &waveform : nullptr, audioProcessor.getSampleRate());
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

        auto* envelopeControl = irControlsComponent.getEnvelopeControl();
        jassert(envelopeControl);
        if (envelopeControl) envelopeControl->setSlot(slot);

        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            auto* swapControl = irControlsComponent.getSwapControl(i);
            jassert(swapControl);
            if (swapControl) {
                swapControl->swapActiveToggle.setVisible(i == selectedIR);
                swapControl->swapRangeSlider.setVisible(i == selectedIR);
            }
        }
    }
}

IRControlsComponent* SelectedIRPanel::getIRControlsComponent() { return &irControlsComponent; }