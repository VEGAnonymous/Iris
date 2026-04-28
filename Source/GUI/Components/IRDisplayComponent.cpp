#include "GUI/Components/IRDisplayComponent.h"

/* PRIVATE */

void IRDisplayComponent::prepare() {
    irWaveformComponent.setDimensions(16.0f, 0.0f, -16.0f, 0.9f);
    irWaveformComponent.setColor(Theme::Colors::highlight);
    addAndMakeVisible(irWaveformComponent);

    windowOverlayComponent.onWindowChanged = [this](float start, float length) {
        audioProcessor.getIRManager()->setWindow(audioProcessor.apvts.state.getProperty(PropertyID::selectedIR), start, length);
    };

    windowOverlayComponent.dragHandler.onFileDropped = [this](const juce::File& file) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        audioProcessor.getIRManager()->loadIR(selectedIR, file);
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
        windowOverlayComponent.dragHover.setAlpha(hovering ? 1.0f : 0.0f);
        windowOverlayComponent.setMouseCursor(hovering ? juce::MouseCursor::CopyingCursor : juce::MouseCursor::NormalCursor);
    };

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
    g.fillRect(bounds.reduced(2));
    g.setColour(juce::Colours::floralwhite.withAlpha(0.4f));
    g.drawRoundedRectangle(bounds.reduced(1).toFloat(), 4.0f, 0.8f);
}

void IRDisplayComponent::resized() {
    irWaveformComponent.setBounds(getLocalBounds());
    windowOverlayComponent.setBounds(irWaveformComponent.getBounds().withTrimmedLeft(16).withTrimmedRight(16));
}

WaveformComponent* IRDisplayComponent::getWaveformComponent() { return &irWaveformComponent; }
WindowOverlayComponent* IRDisplayComponent::getWindowOverlayComponent() { return &windowOverlayComponent; }