#include "GUI/GUIUtilities.h"
#include "GUI/Components/DirectoryManagerComponent.h"

/* PUBLIC */

DirectoryManagerComponent::DirectoryManagerComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater)
    : audioProcessor(processor), animatorUpdater(updater), addButton(updater), removeButton(updater) {

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

    addButton.onClick = [this] {
        audioProcessor.getIRManager()->chooseIRDirectory();
    };

    removeButton.onClick = [this] {
        if (selectedRow >= 1) { // 0 = factory
            audioProcessor.getIRManager()->removeIRDirectory(selectedRow);
            selectedRow = -1;
            directoryList.deselectAllRows();
        }
    };

    directoryList.setRowHeight(DIRECTORY_ROW_HEIGHT);

    randomModeSelector.control.addItemList(randomModes, 1);
    randomModeSelector.control.setSelectedId(
        static_cast<int>(audioProcessor.apvts.state.getProperty(PropertyID::randomIRMode)) + 1, juce::dontSendNotification);
    randomModeSelector.control.onChange = [this]() {
        const int randomMode = randomModeSelector.control.getSelectedId() - 1;
        audioProcessor.apvts.state.setProperty(PropertyID::randomIRMode, randomMode, nullptr);
        audioProcessor.getIRManager()->setRandomMode(static_cast<IRManager::IRSamplingMode>(randomMode));
    };
    addAndMakeVisible(randomModeSelector);
}

void DirectoryManagerComponent::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::section);
    auto bounds = getLocalBounds();
    auto listBounds = bounds
        .withTrimmedTop(TITLE_ROW_HEIGHT)
        .withTrimmedRight(BUTTON_COLUMN_WIDTH + BUTTON_COLUMN_PADDING)
        .withHeight(DIRECTORY_LIST_HEIGHT);

    g.fillRoundedRectangle(listBounds.toFloat(), 12.0f);
}

void DirectoryManagerComponent::resized() {
    auto bounds = getLocalBounds();

    const int titleLabelHeight = 16;
    auto titleBounds = bounds.removeFromTop(titleLabelHeight);
    title.setBounds(titleBounds);

    bounds.removeFromTop(TITLE_ROW_HEIGHT - titleLabelHeight);

    auto buttonBounds = bounds
        .removeFromRight(BUTTON_COLUMN_WIDTH)
        .withHeight(DIRECTORY_LIST_HEIGHT);

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

    auto listBounds = bounds
        .removeFromTop(DIRECTORY_LIST_HEIGHT)
        .withTrimmedRight(BUTTON_COLUMN_PADDING);

    directoryList.setBounds(listBounds);

    bounds.removeFromTop(20);

    randomModeSelector.setLabelDimensions(112.0f, 12.0f);
    randomModeSelector.setControlDimensions(186.0f, 30.0f);
    randomModeSelector.setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f));
    randomModeSelector.setLabelPosition(LabelledControl<juce::ComboBox>::LabelPosition::LEFT);
    randomModeSelector.setBounds(bounds.removeFromTop(CONTROL_COLUMN_HEIGHT));
}

int DirectoryManagerComponent::getNumRows() { return static_cast<int>(audioProcessor.getIRManager()->getIRDirectories().size()); }

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

    const auto& directories = audioProcessor.getIRManager()->getIRDirectories();
    if (rowNumber < static_cast<int>(directories.size())) {
        const auto& directory = directories[rowNumber];

        const int maxPathLength = 62;
        juce::String path = formatPath(directory.irDirectory.getFullPathName(), maxPathLength, Ellipsis::FRONT);
        if (rowNumber == 0) path = "Factory IRs";
        directoryRow->update(path, directory.active);

        const int directoryIndex = rowNumber;
        directoryRow->onToggle = [this, directoryIndex](bool nActive) {
            audioProcessor.getIRManager()->setIRDirectoryActive(directoryIndex, nActive);
        };
    }

    return directoryRow;
}

void DirectoryManagerComponent::refresh() {
    directoryList.updateContent();
    repaint();
}