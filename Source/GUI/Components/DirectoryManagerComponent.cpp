#include "GUI/GUIUtilities.h"
#include "GUI/Components/DirectoryManagerComponent.h"

/* PUBLIC */

DirectoryManagerComponent::DirectoryManagerComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater)
    : audioProcessor(processor), animatorUpdater(updater), addButton(updater), removeButton(updater), refreshButton(updater) {
    prepare();
}

void DirectoryManagerComponent::prepare() {
    setWantsKeyboardFocus(true);

    auto* irManager = audioProcessor.getIRManager();

    // Title
    title.setText("IR Directories", juce::NotificationType::dontSendNotification);
    title.setColour(juce::Label::ColourIds::textColourId, Theme::Colors::highlight);
    title.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(18.0f).withKerningFactor(0.05f)));
    title.setJustificationType(juce::Justification::centredTop);
    title.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(title);

    // Directory list
    directoryList.setColour(juce::ListBox::backgroundColourId, juce::Colours::transparentBlack);
    directoryList.setColour(juce::ListBox::outlineColourId, juce::Colours::transparentBlack);
    directoryList.setRowHeight(DIRECTORY_ROW_HEIGHT);
    addAndMakeVisible(directoryList);

    // Add / remove / refresh buttons
    addButton.setButtonText("+");
    removeButton.setButtonText("-");
    addAndMakeVisible(addButton);
    addAndMakeVisible(removeButton);

    addButton.onClick = [this, irManager]() {
        irManager->chooseIRDirectory();
    };

    removeButton.onClick = [this, irManager]() {
        if (selectedRow >= 0) {
            IRCommand cmd = { IRCommand::IR_DIRECTORY_REMOVE };
            cmd.irDirectoryIndex = selectedRow;
            irManager->enqueueCommand(cmd);
            selectedRow = -1;
            directoryList.deselectAllRows();
        }
    };

    const juce::Image refreshIcon = Theme::Icons::getRefreshIcon();
    const juce::Colour overlayColor = Theme::Colors::highlight;
    refreshButton.setImages(true, true, true,
        refreshIcon, 1.0f, overlayColor,
        refreshIcon, 1.0f, overlayColor,
        refreshIcon, 1.0f, overlayColor);
    refreshButton.onClick = [this, irManager]() {
        if (irManager->getBusyCollecting().load(std::memory_order_acquire)) return;
        IRCommand cmd = { IRCommand::IR_DIRECTORY_REFRESH };
        irManager->enqueueCommand(cmd);
    };
    addAndMakeVisible(refreshButton);

    // File / directory filter editors
    auto setFileFilter = [this, irManager]() {
        if (fileFilterChanged) {
            juce::String nText = fileFilterEditor.control.getText();
            audioProcessor.apvts.state.setProperty(PropertyID::fileFilter, nText, nullptr);

            IRCommand cmd = { IRCommand::SET_FILE_FILTER };
            cmd.fileFilter = nText;
            irManager->enqueueCommand(cmd);

            fileFilterChanged = false;
        }
    };

    auto setDirectoryFilter = [this, irManager]() {
        if (directoryFilterChanged) {
            juce::String nText = directoryFilterEditor.control.getText();
            audioProcessor.apvts.state.setProperty(PropertyID::directoryFilter, nText, nullptr);

            IRCommand cmd = { IRCommand::SET_DIRECTORY_FILTER };
            cmd.directoryFilter = nText;
            irManager->enqueueCommand(cmd);

            directoryFilterChanged = false;
        }
    };

    auto& fileFilterTextEditor = fileFilterEditor.control;
    fileFilterTextEditor.setColour(juce::TextEditor::ColourIds::textColourId, Theme::Colors::textLight);
    fileFilterTextEditor.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(14.0f)));
    fileFilterTextEditor.setText(audioProcessor.apvts.state.getProperty(PropertyID::fileFilter, ""));
    fileFilterTextEditor.setReturnKeyStartsNewLine(false);
    fileFilterTextEditor.setSelectAllWhenFocused(false);
    fileFilterTextEditor.setInputRestrictions(126);
    fileFilterTextEditor.setTextToShowWhenEmpty("Twilight, Moon Dancer, Lyra", Theme::Colors::textDark);
    fileFilterTextEditor.onTextChange = [this]() { fileFilterChanged = true; };
    fileFilterTextEditor.onReturnKey = [this]() { fileFilterEditor.giveAwayKeyboardFocus(); };
    fileFilterTextEditor.onEscapeKey = [this]() { fileFilterEditor.giveAwayKeyboardFocus(); };
    fileFilterTextEditor.onFocusLost = setFileFilter;
    addAndMakeVisible(fileFilterEditor);

    auto& directoryFilterTextEditor = directoryFilterEditor.control;
    directoryFilterTextEditor.setColour(juce::TextEditor::ColourIds::textColourId, Theme::Colors::textLight);
    directoryFilterTextEditor.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(14.0f)));
    directoryFilterTextEditor.setText(audioProcessor.apvts.state.getProperty(PropertyID::directoryFilter, ""));
    directoryFilterTextEditor.setReturnKeyStartsNewLine(false);
    directoryFilterTextEditor.setSelectAllWhenFocused(false);
    directoryFilterTextEditor.setInputRestrictions(126);
    directoryFilterTextEditor.setTextToShowWhenEmpty("S1, S2, S3", Theme::Colors::textDark);
    directoryFilterTextEditor.onTextChange = [this]() { directoryFilterChanged = true; };
    directoryFilterTextEditor.onReturnKey = [this]() { directoryFilterEditor.giveAwayKeyboardFocus(); };
    directoryFilterTextEditor.onEscapeKey = [this]() { directoryFilterEditor.giveAwayKeyboardFocus(); };
    directoryFilterTextEditor.onFocusLost = setDirectoryFilter;
    addAndMakeVisible(directoryFilterEditor);

    // Sampling mode selector
    samplingModeSelector.control.addItemList(randomModes, 1);
    samplingModeSelector.control.setSelectedId(
        static_cast<int>(audioProcessor.apvts.state.getProperty(PropertyID::samplingMode)) + 1, juce::dontSendNotification);
    samplingModeSelector.control.onChange = [this]() {
        const int randomMode = samplingModeSelector.control.getSelectedId() - 1;
        audioProcessor.apvts.state.setProperty(PropertyID::samplingMode, randomMode, nullptr);

        IRCommand cmd = { IRCommand::SET_SAMPLING_MODE };
        cmd.samplingMode = static_cast<IRSamplingMode>(randomMode);
        audioProcessor.getIRManager()->enqueueCommand(cmd);
    };
    addAndMakeVisible(samplingModeSelector);
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

    buttonColumn.items.add(juce::FlexItem(refreshButton)
        .withWidth(BUTTON_COLUMN_WIDTH - buttonMargin - 3.0f)
        .withHeight(buttonHeight - 2.0f)
        .withMargin(buttonMargin));

    buttonColumn.performLayout(buttonBounds);

    auto listBounds = bounds
        .removeFromTop(DIRECTORY_LIST_HEIGHT)
        .withTrimmedRight(BUTTON_COLUMN_PADDING);

    directoryList.setBounds(listBounds);

    bounds.removeFromTop(20);

    juce::FlexBox controlColumn(juce::FlexBox::JustifyContent::center);
    controlColumn.flexDirection = juce::FlexBox::Direction::column;
    controlColumn.alignItems = juce::FlexBox::AlignItems::center;

    fileFilterEditor.setLabelDimensions(112.0f, 12.0f);
    fileFilterEditor.setControlDimensions(186.0f, 20.0f);
    fileFilterEditor.setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f));
    fileFilterEditor.setLabelPosition(LabelledControl<HoverableTextEditor>::LabelPosition::LEFT);
    controlColumn.items.add(juce::FlexItem(fileFilterEditor)
        .withFlex(0.0f)
        .withWidth(static_cast<float>(bounds.getWidth()))
        .withHeight(40.0f)
        .withMargin(10.0f));

    directoryFilterEditor.setLabelDimensions(112.0f, 12.0f);
    directoryFilterEditor.setControlDimensions(186.0f, 20.0f);
    directoryFilterEditor.setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f));
    directoryFilterEditor.setLabelPosition(LabelledControl<HoverableTextEditor>::LabelPosition::LEFT);
    controlColumn.items.add(juce::FlexItem(directoryFilterEditor)
        .withFlex(0.0f)
        .withWidth(static_cast<float>(bounds.getWidth()))
        .withHeight(40.0f)
        .withMargin(10.0f));

    samplingModeSelector.setLabelDimensions(112.0f, 12.0f);
    samplingModeSelector.setControlDimensions(186.0f, 30.0f);
    samplingModeSelector.setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f));
    samplingModeSelector.setLabelPosition(LabelledControl<juce::ComboBox>::LabelPosition::LEFT);
    controlColumn.items.add(juce::FlexItem(samplingModeSelector)
        .withFlex(0.0f)
        .withWidth(static_cast<float>(bounds.getWidth()))
        .withHeight(40.0f)
        .withMargin(10.0f));

    controlColumn.performLayout(bounds.removeFromTop(CONTROL_COLUMN_HEIGHT));
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
    if (!directoryRow) directoryRow = new DirectoryRowComponent(animatorUpdater, audioProcessor.getIRManager()->getBusyCollecting());

    const auto& directories = audioProcessor.getIRManager()->getIRDirectories();
    if (rowNumber < static_cast<int>(directories.size())) {
        const auto& directory = directories[rowNumber];

        const int maxPathLength = 62;
        juce::String path = formatPath(directory.irDirectory.getFullPathName(), maxPathLength, Ellipsis::FRONT);
        directoryRow->update(path, directory.active);

        const int directoryIndex = rowNumber;
        directoryRow->onToggle = [this, directoryIndex](bool nActive) {
            IRCommand cmd = { IRCommand::IR_DIRECTORY_SET_ACTIVE_STATE };
            cmd.irDirectoryIndex = directoryIndex;
            cmd.irDirectoryActiveState = nActive;
            audioProcessor.getIRManager()->enqueueCommand(cmd);
        };
    }

    return directoryRow;
}

void DirectoryManagerComponent::refresh() {
    bool busyCollecting = audioProcessor.getIRManager()->getBusyCollecting().load(std::memory_order_acquire);

    refreshButton.setEnabled(!busyCollecting);
    fileFilterEditor.setEnabled(!busyCollecting);
    directoryFilterEditor.setEnabled(!busyCollecting);

    const float busyAlpha = 0.4f;
    refreshButton.setAlpha(busyCollecting ? busyAlpha : 1.0f);
    fileFilterEditor.setAlpha(busyCollecting ? busyAlpha : 1.0f);
    directoryFilterEditor.setAlpha(busyCollecting ? busyAlpha : 1.0f);

    directoryList.updateContent();
    repaint();
}