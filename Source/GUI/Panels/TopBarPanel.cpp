#include "GUI/Panels/TopBarPanel.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void TopBarPanel::prepare() {
    // Randomize / clear all buttons
    randomAllButton.setButtonText("RANDOM ALL");
    randomAllButton.onClick = [irManager = audioProcessor.getIRManager()]() {
        if (irManager->getBusyLoading().load(std::memory_order_acquire)) return; // NICE TRY
        IRCommand cmd = { IRCommand::IR_LOAD_RANDOM_ALL };
        irManager->enqueueCommand(cmd);
    };
    addAndMakeVisible(randomAllButton);

    clearAllButton.setButtonText("CLEAR ALL");
    clearAllButton.onClick = [irManager = audioProcessor.getIRManager()]() {
        IRCommand cmd = { IRCommand::IR_CLEAR_ALL };
        irManager->enqueueCommand(cmd);
    };
    addAndMakeVisible(clearAllButton);

    // Directory manager modal
    const juce::Image directoryManagerIcon = Theme::Icons::getMenuIcon();
   juce::Colour overlayColor = Theme::Colors::highlight;
    directoryManagerButton.setImages(true, true, true,
        directoryManagerIcon, 1.0f, overlayColor,
        directoryManagerIcon, 1.0f, overlayColor,
        directoryManagerIcon, 1.0f, overlayColor);
    directoryManagerButton.onClick = [&]() { if (onDirectoryManagerClicked) onDirectoryManagerClicked(); };
    addAndMakeVisible(directoryManagerButton);

    // Settings modal
    const juce::Image settingsIcon = Theme::Icons::getSettingsIcon();
    settingsButton.setImages(true, true, true,
        settingsIcon, 1.0f, overlayColor,
        settingsIcon, 1.0f, overlayColor,
        settingsIcon, 1.0f, overlayColor);
    settingsButton.onClick = [&]() { if (onSettingsClicked) onSettingsClicked(); };
    addAndMakeVisible(settingsButton);

    // Logo
    sourceRNG = juce::Random();
    sourceRNG.setSeedRandomly();

    const juce::Image logoIcon = Theme::Icons::getLogo();
    overlayColor = Theme::Colors::disabled;
    logo.setColorOnHover(overlayColor.brighter(0.1f));
    logo.setImages(true, true, true,
        logoIcon, 1.0f, overlayColor,
        logoIcon, 1.0f, overlayColor,
        logoIcon, 1.0f, overlayColor);
    logo.onClick = [&]() {
        const juce::String source = sources[sourceRNG.nextInt(sources.size())];
        juce::URL pageURL(source);
        if (pageURL.isWellFormed() && juce::URL::isProbablyAWebsiteURL(source)) {
            pageURL.launchInDefaultBrowser();
        }
    };
    addAndMakeVisible(logo);
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

    topBarRow.items.add(juce::FlexItem(directoryManagerButton)
        .withFlex(0.0f)
        .withWidth(w * 0.075f)
        .withHeight(h * 0.8f)
        .withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f)));

    topBarRow.items.add(juce::FlexItem(settingsButton)
        .withFlex(0.0f)
        .withWidth(w * 0.075f)
        .withHeight(h * 0.3f)
        .withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 20.0f)));

    topBarRow.performLayout(bounds.removeFromLeft(static_cast<int>(w * 0.6f)));
    
    bounds.removeFromRight(10);
    logo.setBounds(bounds.removeFromRight(80));
}

void TopBarPanel::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds();
    g.setColour(Theme::Colors::background);
    g.fillRect(getLocalBounds());
}