#include "TopBarPanel.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void TopBarPanel::prepare() {
    // Randomize / clear all buttons
    randomAllButton.setButtonText("RANDOM ALL");
    randomAllButton.onClick = [this]() { audioProcessor.getIRManager()->loadRandomIRs(); };
    addAndMakeVisible(randomAllButton);

    clearAllButton.setButtonText("CLEAR ALL");
    clearAllButton.onClick = [this]() { audioProcessor.getIRManager()->clearIRs(); };
    addAndMakeVisible(clearAllButton);

    // Settings modal
    const juce::Image settingsIcon = Theme::Icons::getBurgerMenuIcon();
    const juce::Colour overlayColor = Theme::Colors::highlight;
    settingsButton.setImages(true, true, true,
        settingsIcon, 1.0f, overlayColor,
        settingsIcon, 1.0f, overlayColor,
        settingsIcon, 1.0f, overlayColor);
    settingsButton.onClick = [&]() { if (onSettingsClicked) onSettingsClicked(); };
    addAndMakeVisible(settingsButton);
}

/* PUBLIC */

TopBarPanel::TopBarPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater)
    : audioProcessor(processor), animatorUpdater(updater) {
    prepare();
}

void TopBarPanel::resized() {
    Bounds bounds = getLocalBounds();
    const auto w = bounds.getWidth(),
               h = bounds.getHeight();

    juce::FlexBox topBarRow(juce::FlexBox::JustifyContent::flexStart);
    topBarRow.alignItems = juce::FlexBox::AlignItems::center;

    juce::FlexBox globalIROperations(juce::FlexBox::JustifyContent::center);
    globalIROperations.flexDirection = juce::FlexBox::Direction::column;

    globalIROperations.items.add(juce::FlexItem(randomAllButton).withFlex(1.0f));
    globalIROperations.items.add(juce::FlexItem(clearAllButton).withFlex(1.0f));

    topBarRow.items.add(juce::FlexItem(globalIROperations)
        .withFlex(0.0f)
        .withWidth(w * 0.15f)
        .withHeight(h * 0.8f)
        .withMargin(22.5f));

    topBarRow.items.add(juce::FlexItem(settingsButton)
        .withFlex(0.0f)
        .withWidth(w * 0.075f)
        .withHeight(h * 0.6f)
        .withMargin(8.0f));

    topBarRow.performLayout(bounds.removeFromLeft(static_cast<int>(w * 0.4f)));
}

void TopBarPanel::paint(juce::Graphics& g) {
    g.setColour(Theme::Colors::background);
    g.fillRect(getLocalBounds());
}