#include "DirectoryManagerComponent.h"

DirectoryManagerComponent::DirectoryManagerComponent(IRManager* manager) : irManager(manager) {
    addAndMakeVisible(directoryList);
    addAndMakeVisible(addButton);
    addAndMakeVisible(removeButton);
}

void DirectoryManagerComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkslategrey); // TEMP
    // TODO
}

void DirectoryManagerComponent::resized() {
    // TODO
}

int DirectoryManagerComponent::getNumRows() { return 0; } // TEMP

void DirectoryManagerComponent::paintListBoxItem(int, juce::Graphics&, int, int, bool) {}

void DirectoryManagerComponent::listBoxItemClicked(int row, const juce::MouseEvent&) { selectedRow = row; }

void DirectoryManagerComponent::refresh() {
    directoryList.updateContent();
    repaint();
}