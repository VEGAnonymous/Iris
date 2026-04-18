#include "DirectoryManagerComponent.h"
#include "GUIUtilities.h"

DirectoryManagerComponent::DirectoryManagerComponent(IRManager* manager) : irManager(manager) {
    addAndMakeVisible(directoryList);
    addAndMakeVisible(addButton);
    addAndMakeVisible(removeButton);

    directoryList.setRowHeight(28);

    addButton.onClick = [this] {
        this->irManager->chooseIRDirectory();
    };

    removeButton.onClick = [this] {
        if (selectedRow >= 1) { // 0 = factory
            this->irManager->removeIRDirectory(selectedRow);
            selectedRow = -1;
            directoryList.deselectAllRows();
        }
    };
}

void DirectoryManagerComponent::paint(juce::Graphics& g) {
    g.fillAll(juce::Colours::darkslategrey);
}

void DirectoryManagerComponent::resized() {
    auto bounds = getLocalBounds();

    auto buttonColumn = bounds.removeFromRight(32);
    addButton.setBounds(buttonColumn.removeFromTop(32));
    removeButton.setBounds(buttonColumn.removeFromTop(32));

    directoryList.setBounds(bounds);
}

int DirectoryManagerComponent::getNumRows() { return static_cast<int>(irManager->getIRDirectories().size()); }

void DirectoryManagerComponent::paintListBoxItem(int /*rowNumber*/, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    if (rowIsSelected) {
        g.setColour(juce::Colours::lightblue.withAlpha(0.4f));
        g.fillRect(0, 0, width, height);
    }
}

void DirectoryManagerComponent::listBoxItemClicked(int row, const juce::MouseEvent&) { selectedRow = row; }

juce::Component* DirectoryManagerComponent::refreshComponentForRow(int rowNumber, 
    bool /*isRowSelected*/, juce::Component* existingComponentToUpdate) {
    auto* directoryRow = dynamic_cast<DirectoryRowComponent*>(existingComponentToUpdate);
    if (!directoryRow) directoryRow = new DirectoryRowComponent();

    const auto& directories = irManager->getIRDirectories();
    if (rowNumber < static_cast<int>(directories.size())) {
        const auto& directory = directories[rowNumber];

        juce::String path = formatPath(directory.irDirectory);
        if (rowNumber == 0) path = "Factory IRs";
        directoryRow->update(path, directory.active);

        const int directoryIndex = rowNumber;
        directoryRow->onToggle = [this, directoryIndex](bool nActive) {
            irManager->setIRDirectoryActive(directoryIndex, nActive);
        };
    }

    return directoryRow;
}


void DirectoryManagerComponent::refresh() {
    directoryList.updateContent();
    repaint();
}