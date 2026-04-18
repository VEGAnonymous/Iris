#include "SettingsComponent.h"

SettingsComponent::SettingsComponent(IRManager* irManager) : directoryManager(irManager) {
    addAndMakeVisible(closeButton);
    addAndMakeVisible(directoryManager);

    closeButton.onClick = [this] {
        if (onCloseRequested) onCloseRequested();
    };
}

void SettingsComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::black.withAlpha(0.92f));
    g.setColour(juce::Colours::grey);
    g.drawRect(getLocalBounds(), 1);
}

void SettingsComponent::resized() {
    auto bounds = getLocalBounds().reduced(8);
    closeButton.setBounds(bounds.removeFromTop(24).removeFromRight(24));
    directoryManager.setBounds(bounds);
}

void SettingsComponent::refreshDirectories() { directoryManager.refresh(); }