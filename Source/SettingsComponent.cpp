#include "SettingsComponent.h"
#include "Theme.h"

SettingsComponent::SettingsComponent(IRManager* irManager, juce::AnimatorUpdater& updater) 
    : directoryManager(irManager, updater), closeButton(updater) {
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
    auto bounds = getLocalBounds().reduced(16);
    closeButton.setBounds(bounds.removeFromTop(24).removeFromRight(24));
    directoryManager.setBounds(bounds.reduced(8));
}

void SettingsComponent::refreshDirectories() { directoryManager.refresh(); }