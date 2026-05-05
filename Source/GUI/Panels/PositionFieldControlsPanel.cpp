#include "GUI/Panels/PositionFieldControlsPanel.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void PositionFieldControlsPanel::prepare() {
    addAndMakeVisible(positionTabButton);
    addAndMakeVisible(fieldTabButton);

    for (auto& controlTab : controlTabs) {
        addAndMakeVisible(controlTab.pattern);
        for (auto* rotary : controlTab.rotaries)
            addAndMakeVisible(rotary);
    }

    // Tab selection
    auto selectTab = [this](ControlTabID nActiveTab) {
        activeTab = nActiveTab;

        positionTabButton.setToggleState(activeTab == ControlTabID::POSITION, juce::dontSendNotification);
        fieldTabButton.setToggleState(activeTab == ControlTabID::FIELD, juce::dontSendNotification);
        
        for (int tab = 0; tab < controlTabs.size(); ++tab) {
            bool visible = (tab == activeTab);
            controlTabs[tab].pattern->setVisible(visible);
            for (auto* rotary : controlTabs[tab].rotaries)
                rotary->setVisible(visible);
        }
        resized();
    };

    positionTabButton.setButtonText("POSITION");
    positionTabButton.setToggleable(true);
    positionTabButton.onClick = [this, selectTab]() {
        selectTab(ControlTabID::POSITION); 
        audioProcessor.apvts.state.setProperty(PropertyID::selectedControlTab, ControlTabID::POSITION, nullptr);
        audioProcessor.guiState.syncingPosition.store(true, std::memory_order_release);
    };

    fieldTabButton.setButtonText("FIELD");
    fieldTabButton.setToggleable(true);
    fieldTabButton.onClick = [this, selectTab]() { 
        selectTab(ControlTabID::FIELD); 
        audioProcessor.apvts.state.setProperty(PropertyID::selectedControlTab, ControlTabID::FIELD, nullptr);
        audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
    };

    selectTab(static_cast<ControlTabID>(static_cast<int>(audioProcessor.apvts.state.getProperty(PropertyID::selectedControlTab))));

    // Pattern combo boxes
    auto* positionPatternControl = controlTabs[ControlTabID::POSITION].pattern;
    positionPatternControl->control.addItemList(positionPatterns, 1);
    positionPatternControl->control.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load()) + 1, juce::dontSendNotification);

    auto* fieldPatternControl = controlTabs[ControlTabID::FIELD].pattern;
    fieldPatternControl->control.addItemList(fieldPatterns, 1);
    fieldPatternControl->control.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load()) + 1, juce::dontSendNotification);
}

/* PUBLIC */

PositionFieldControlsPanel::PositionFieldControlsPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, 
    std::array<ControlTab, 2> tabs)
    : audioProcessor(processor), animatorUpdater(updater), controlTabs(tabs) {
    prepare();
}

void PositionFieldControlsPanel::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::section);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), PANEL_CORNER_SIZE);
}

void PositionFieldControlsPanel::resized() {
    Bounds bounds = getLocalBounds();
    // Position / field tab selector column
    juce::FlexBox positionFieldTabs(juce::FlexBox::JustifyContent::center);
    positionFieldTabs.flexDirection = juce::FlexBox::Direction::column;

    positionFieldTabs.items.add(juce::FlexItem(positionTabButton)
        .withFlex(1.0f)
        .withMargin(juce::FlexItem::Margin(10.0f, 8.0f, 5.0f, 10.0f)));

    positionFieldTabs.items.add(juce::FlexItem(fieldTabButton)
        .withFlex(1.0f)
        .withMargin(juce::FlexItem::Margin(5.0f, 8.0f, 10.0f, 10.0f)));

    positionFieldTabs.performLayout(bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.2f)));

    // Control row
    const float comboBoxWidth = 112.0f, comboBoxHeight = 30.0f;
    const float rotaryWidth = 70.0f, rotaryHeight = 80.0f;
    const float labelHeight = 12.0f;
    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f);

    auto& controlTab = controlTabs[activeTab];
    juce::FlexBox controlRow(juce::FlexBox::JustifyContent::flexEnd);
    controlRow.alignItems = juce::FlexBox::AlignItems::center;

    // Pattern selector
    auto* patternControl = controlTab.pattern;
    patternControl->setLabelDimensions(comboBoxWidth - 36.0f, labelHeight);
    patternControl->setControlDimensions(comboBoxWidth, comboBoxHeight);
    patternControl->setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 18.0f, 0.0f));
    patternControl->flex.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    controlRow.items.add(juce::FlexItem(*patternControl)
        .withFlex(1.0f)
        .withWidth(comboBoxWidth)
        .withHeight(80.0f)
        .withMargin(rowItemMargin));

    // Knobs
    for (auto* rotary : controlTab.rotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        controlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

    controlRow.performLayout(bounds);
}