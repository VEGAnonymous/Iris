#include "GUI/Panels/InteractionControlsPanel.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void InteractionControlsPanel::prepare() {
    addAndMakeVisible(interactionControls.weightingControl);
    for (auto* rotary : interactionControls.rotaries)
        addAndMakeVisible(rotary);
}

/* PUBLIC */

InteractionControlsPanel::InteractionControlsPanel(InteractionRow controls) : interactionControls(controls) {
    prepare();
}

void InteractionControlsPanel::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::section);
    g.fillRoundedRectangle(getLocalBounds().toFloat(), PANEL_CORNER_SIZE);
}

void InteractionControlsPanel::resized() {
    const Bounds bounds = getLocalBounds();
    juce::FlexBox interactionControlRow(juce::FlexBox::JustifyContent::center);
    interactionControlRow.alignItems = juce::FlexBox::AlignItems::center;

    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 30.0f, 10.0f, 30.0f);
    const float labelHeight = 12.0f;
    const float rotaryWidth = 70.0f, rotaryHeight = 80.0f;

    // Weighting mode
    auto* weightingControl = interactionControls.weightingControl;
    // TEMP: This is currently a LabelledControl<HoverableTextButton> but will later be a toggle switch-like component
    weightingControl->setLabelDimensions(68.0f, 12.0f);
    weightingControl->setControlDimensions(70.0f, 40.0f);
    weightingControl->setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 15.0f, 0.0f));
    weightingControl->flex.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    interactionControlRow.items.add(juce::FlexItem(*weightingControl)
        .withFlex(0.0f)
        .withWidth(68.0f)
        .withHeight(80.0f)
        .withMargin(rowItemMargin));

    // Knobs
    for (auto* rotary : interactionControls.rotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        interactionControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

    interactionControlRow.performLayout(bounds);
}