#include "GUI/Panels/GlobalControlsPanel.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void GlobalControlsPanel::prepare() {
    for (auto* rotary : globalControls.rotaries) 
        addAndMakeVisible(rotary);
}

/* PUBLIC */

GlobalControlsPanel::GlobalControlsPanel(GlobalRow controls) : globalControls(controls) {
    prepare();
}

void GlobalControlsPanel::paint(juce::Graphics& g) {
	g.setColour(Theme::Colors::section);
	g.fillRect(getLocalBounds().reduced(2));
}

void GlobalControlsPanel::resized() {
    const Bounds bounds = getLocalBounds();
    juce::FlexBox globalControlRow(juce::FlexBox::JustifyContent::center);
    globalControlRow.alignItems = juce::FlexBox::AlignItems::center;

    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f);
    const float labelHeight = 12.0f;
    const float rotaryWidth = 70.0f, rotaryHeight = 80.0f;
    
    // Knobs
    for (auto* rotary : globalControls.rotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        globalControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

    globalControlRow.performLayout(bounds);
}