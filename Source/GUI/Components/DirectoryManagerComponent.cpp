#include "GUI/GUIUtilities.h"
#include "GUI/Components/DirectoryManagerComponent.h"

/* PUBLIC */

DirectoryManagerComponent::DirectoryManagerComponent(IRManager* manager, juce::AnimatorUpdater& updater) 
    : irManager(manager), animatorUpdater(updater), addButton(updater), removeButton(updater) {

    title.setText("IR Directories", juce::NotificationType::dontSendNotification);
    title.setColour(juce::Label::ColourIds::textColourId, Theme::Colors::textLight);
    title.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(18.0f).withKerningFactor(0.05f)));
    title.setJustificationType(juce::Justification::centredTop);
    title.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(title);

    directoryList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    directoryList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible(directoryList);

    addButton.setButtonText("+");
    removeButton.setButtonText("-");
    addAndMakeVisible(addButton);
    addAndMakeVisible(removeButton);

    directoryList.setRowHeight(DIRECTORY_ROW_HEIGHT);

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

    setColour(juce::ScrollBar::ColourIds::thumbColourId, Theme::Colors::highlight);
}

void DirectoryManagerComponent::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::section);
    g.fillRoundedRectangle(getLocalBounds()
        .withTrimmedTop(TITLE_ROW_HEIGHT)
        .withTrimmedRight(BUTTON_COLUMN_WIDTH + BUTTON_COLUMN_PADDING)
        .toFloat(), 12.0f);
}

void DirectoryManagerComponent::resized() {
    auto bounds = getLocalBounds();
    const int titleLabelHeight = 16;
    auto titleBounds = bounds.removeFromTop(titleLabelHeight);
    title.setBounds(titleBounds);

    bounds.removeFromTop(TITLE_ROW_HEIGHT - titleLabelHeight);
    auto buttonBounds = bounds.removeFromRight(BUTTON_COLUMN_WIDTH);
    const float buttonHeight = 70, buttonMargin = 2.0f;

    juce::FlexBox buttonColumn(juce::FlexBox::JustifyContent::center);
    buttonColumn.flexDirection = juce::FlexBox::Direction::column;
    buttonColumn.alignItems = juce::FlexBox::AlignItems::center;

    buttonColumn.items.add(juce::FlexItem(addButton)
        .withWidth(BUTTON_COLUMN_WIDTH - buttonMargin)
        .withHeight(buttonHeight)
        .withMargin(buttonMargin));
    buttonColumn.items.add(juce::FlexItem(removeButton)
        .withWidth(BUTTON_COLUMN_WIDTH - buttonMargin)
        .withHeight(buttonHeight)
        .withMargin(buttonMargin));

    buttonColumn.performLayout(buttonBounds);

    directoryList.setBounds(bounds.withTrimmedRight(BUTTON_COLUMN_PADDING));
}

int DirectoryManagerComponent::getNumRows() { return static_cast<int>(irManager->getIRDirectories().size()); }

void DirectoryManagerComponent::paintListBoxItem(int /*rowNumber*/, juce::Graphics& g, int width, int height, bool rowIsSelected) {
    g.setColour(rowIsSelected ? Theme::Colors::highlight.withAlpha(0.1f) : Theme::Colors::section);
    g.fillRoundedRectangle(Bounds(0, 0, width, height).toFloat(), 12.0f);
}

void DirectoryManagerComponent::listBoxItemClicked(int row, const juce::MouseEvent&) { selectedRow = row; }

juce::Component* DirectoryManagerComponent::refreshComponentForRow(int rowNumber, 
    bool /*isRowSelected*/, juce::Component* existingComponentToUpdate) {
    // Probably UNSAFE
    auto* directoryRow = dynamic_cast<DirectoryRowComponent*>(existingComponentToUpdate);
    if (!directoryRow) directoryRow = new DirectoryRowComponent(animatorUpdater);

    const auto& directories = irManager->getIRDirectories();
    if (rowNumber < static_cast<int>(directories.size())) {
        const auto& directory = directories[rowNumber];

        const int maxPathLength = 62;
        juce::String path = formatPath(directory.irDirectory.getFullPathName(), maxPathLength, Ellipsis::FRONT);
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