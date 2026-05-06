#include "GUI/Components/IRDisplayComponent.h"

/* PRIVATE */

void IRDisplayComponent::prepare() {
    irWaveformComponent.setDimensions(16.0f, 0.0f, -16.0f, 0.9f);
    irWaveformComponent.setNumPoints(WAVEFORM_POINTS);
    irWaveformComponent.setColor(Theme::Colors::highlight);
    addAndMakeVisible(irWaveformComponent);

    windowOverlayComponent.onWindowChanged = [this](float start, float length) {
        IRCommand cmd = { IRCommand::IR_SET_WINDOW };
        cmd.irIndex = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        cmd.windowStart = start;
        cmd.windowLength = length;
        audioProcessor.getIRManager()->enqueueCommand(cmd);
    };

    windowOverlayComponent.dragHandler.onFileDropped = [this](const juce::File& file) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        IRCommand cmd = { IRCommand::IR_LOAD };
        cmd.irIndex = selectedIR;
        cmd.irFile = file;
        audioProcessor.getIRManager()->enqueueCommand(cmd);
    };

    windowOverlayComponent.dragHandler.fileFilter = [this](const juce::File& file) {
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

    windowOverlayComponent.dragHandler.onHoverChanged = [this](bool hovering) {
        windowOverlayComponent.dragHover.setValue(hovering ? 1.0f : 0.0f);
        windowOverlayComponent.setMouseCursor(hovering ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::NormalCursor);
    };

    windowOverlayComponent.setTooltip("Drag the handles or drag-select to set the convolvable range of the selected IR. For performance reasons, the range size is capped at ~6s @ 44.1kHz and the maximum IR duration is ~60s @ 44.1kHz.\n\nCan accept audio files via drag-and-drop.");

    addAndMakeVisible(windowOverlayComponent);
}

/* PUBLIC */

IRDisplayComponent::IRDisplayComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater) 
    : audioProcessor(processor), animatorUpdater(updater),
      irWaveformComponent(updater), windowOverlayComponent(updater) {
    prepare();
}

void IRDisplayComponent::paint(juce::Graphics& g) {
    Bounds bounds = getLocalBounds();
    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(bounds.toFloat(), PANEL_CORNER_SIZE);
    g.setColour(juce::Colours::floralwhite.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(1).toFloat(), 4.0f, 0.8f);
}

void IRDisplayComponent::resized() {
    irWaveformComponent.setBounds(getLocalBounds());
    windowOverlayComponent.setBounds(irWaveformComponent.getBounds().withTrimmedLeft(16).withTrimmedRight(16));
}

WaveformComponent* IRDisplayComponent::getWaveformComponent() { return &irWaveformComponent; }
WindowOverlayComponent* IRDisplayComponent::getWindowOverlayComponent() { return &windowOverlayComponent; }