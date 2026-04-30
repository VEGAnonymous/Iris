#include "GUI/Components/SettingsComponent.h"
#include "GUI/Theme/Theme.h"

/* PUBLIC */

SettingsComponent::SettingsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater)
    : directoryManager(processor, updater), closeButton(updater) {
    addAndMakeVisible(closeButton);
    addAndMakeVisible(directoryManager);
    directoryManager.refresh();

    closeButton.setButtonText("X");
    closeButton.onClick = [this] {
        if (onCloseRequested) onCloseRequested();
    };
}

void SettingsComponent::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), 12.0f);
}

void SettingsComponent::resized() {
    auto bounds = getLocalBounds().reduced(SETTINGS_INSET);
    // closeButton.setBounds(bounds.removeFromTop(24).removeFromRight(24)); // TEMP: Likely don't need this
    directoryManager.setBounds(bounds.withTrimmedLeft(8).withTrimmedTop(8).withTrimmedBottom(8));
}

void SettingsComponent::refreshDirectories() { directoryManager.refresh(); }