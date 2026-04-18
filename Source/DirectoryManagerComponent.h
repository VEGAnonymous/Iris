#pragma once

#include "IRManager.h"

#include <JuceHeader.h>

class DirectoryManagerComponent : public juce::Component, public juce::ListBoxModel {
private:
    IRManager* irManager;

    juce::ListBox directoryList{ {}, this };
    juce::TextButton addButton{ "+" }, removeButton{ "-" };

    int selectedRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectoryManagerComponent)

public:
    DirectoryManagerComponent(IRManager* irManager);
    ~DirectoryManagerComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;

    void refresh();
};