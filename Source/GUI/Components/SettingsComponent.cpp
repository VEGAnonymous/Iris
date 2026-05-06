#include "GUI/Components/SettingsComponent.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void SettingsComponent::prepare() {
    auto& state = audioProcessor.apvts.state;

    // Title + details
    title.setText(juce::String::fromUTF8(ProjectInfo::projectName), 
        juce::dontSendNotification);
    title.setColour(juce::Label::ColourIds::textColourId, Theme::Colors::highlight);
    title.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(18.0f).withKerningFactor(0.05f)));
    title.setJustificationType(juce::Justification::centredTop);
    title.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(title);

    details.setText(juce::String::fromUTF8(ProjectInfo::companyName) + "; " + juce::String::fromUTF8(ProjectInfo::versionString), 
        juce::dontSendNotification);
    details.setColour(juce::Label::ColourIds::textColourId, Theme::Colors::textLight);
    details.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(12.0f).withKerningFactor(0.05f)));
    details.setJustificationType(juce::Justification::centredTop);
    details.setMinimumHorizontalScale(1.0f);
    addAndMakeVisible(details);

    // Tooltip toggle
    tooltipToggle.control.setToggleState(state.getProperty(PropertyID::tooltipsEnabled, false), juce::dontSendNotification);
    tooltipToggle.control.onStateChange = [this]() {
        audioProcessor.apvts.state.setProperty(PropertyID::tooltipsEnabled, tooltipToggle.control.getToggleState(), nullptr);
        audioProcessor.guiState.tooltipsEnabledChanged.store(true, std::memory_order_release);
    };
    tooltipToggle.control.setTooltip("Enables info tooltips on hover, like the one you're looking at right now.");
    addAndMakeVisible(tooltipToggle);

    // Position indicator selector
    positionIndicatorSelector.control.addItemList(positionIndicatorStyles, 1);
    positionIndicatorSelector.control.setSelectedId(juce::jmax(1, positionIndicatorStyles.indexOf(
        juce::StringRef(state.getProperty(PropertyID::positionIndicatorStyle, PositionIndicatorStyle::Anon))) + 1), juce::dontSendNotification);
    positionIndicatorSelector.control.onChange = [this]() {
        const int selectedIndex = positionIndicatorSelector.control.getSelectedId() - 1;
        audioProcessor.apvts.state.setProperty(PropertyID::positionIndicatorStyle, positionIndicatorStyles[selectedIndex], nullptr);
        audioProcessor.guiState.indicatorStyleChanged.store(true, std::memory_order_release);
    };
    positionIndicatorSelector.control.setTooltip("Choose your fighter!");
    addAndMakeVisible(positionIndicatorSelector);

    // Field indicator selector
    fieldIndicatorSelector.control.addItemList(fieldIndicatorStyles, 1);
    fieldIndicatorSelector.control.setSelectedId(juce::jmax(1, fieldIndicatorStyles.indexOf(
        juce::StringRef(state.getProperty(PropertyID::fieldIndicatorStyle, FieldIndicatorStyle::Mareful))) + 1), juce::dontSendNotification);
    fieldIndicatorSelector.control.onChange = [this]() {
        const int selectedIndex = fieldIndicatorSelector.control.getSelectedId();
        audioProcessor.apvts.state.setProperty(PropertyID::fieldIndicatorStyle, fieldIndicatorStyles[selectedIndex - 1], nullptr);
        audioProcessor.guiState.indicatorStyleChanged.store(true, std::memory_order_release);
    };
    fieldIndicatorSelector.control.setTooltip("How much mare do you want?\n\nMareful will attempt to match the filename of each IR to a mare (optimized for Clipper's file naming system) and fall back to Anonfilly if none match.\n\nHalf-Mared will attempt the same but fall back to lame dot indicators.\n\n>Mareless\nDYKWYA?");
    addAndMakeVisible(fieldIndicatorSelector);

    // Control rate selector
    controlRateSelector.control.addItemList(controlRates, 1);
    controlRateSelector.control.setSelectedId(static_cast<int>(state.getProperty(PropertyID::controlRate, 3 /* 30 Hz */)), juce::dontSendNotification);
    controlRateSelector.control.onChange = [this]() {
        const int selectedIndex = controlRateSelector.control.getSelectedId();
        audioProcessor.apvts.state.setProperty(PropertyID::controlRate, selectedIndex, nullptr);

        const float controlRate = controlRates[selectedIndex - 1].removeCharacters(" Hz").getFloatValue();
        audioProcessor.setControlRate(controlRate);
    };
    controlRateSelector.control.setTooltip("The control rate determines how often updates to the motion and convolution state (distances, weighting, re-mixing the IRs) are polled.\n\nHigher values have higher \"resolution\" and are thus more responsive to fast and/or sudden changes in the IRs, but also run the risk of lagging out if performance deadlines are not met (e.g., @ 40 Hz, 25 ms).\n\nOn the other hoof, lower values may improve reliability (NOT audio performance) but have somewhat less responsiveness to rapid changes.");
    addAndMakeVisible(controlRateSelector);

    // LYRALYRALYRALYRALYRA
    lyraRNG = juce::Random();
    lyraRNG.setSeedRandomly();
    lyraImage = Theme::Images::getLyra(lyraRNG.nextInt(MareverbLyras::namedResourceListSize));
}

/* PUBLIC */

SettingsComponent::SettingsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater) 
    : audioProcessor(processor), animatorUpdater(updater) {
    prepare();
}

SettingsComponent::~SettingsComponent() {
    // Clean up callbacks
    tooltipToggle.control.onStateChange = nullptr;
    positionIndicatorSelector.control.onChange = nullptr;
    fieldIndicatorSelector.control.onChange = nullptr;
    controlRateSelector.control.onChange = nullptr;
}

void SettingsComponent::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    const float cornerSize = 12.0f;

    g.setColour(Theme::Colors::outline);
    g.drawRoundedRectangle(bounds, cornerSize, 4.0f);

    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(bounds.reduced(1.0f), cornerSize);

    g.drawImage(
        lyraImage, 
        bounds.withTrimmedTop(28 + (PADDING * 4) + CONTROL_COLUMN_HEIGHT).reduced(30).toFloat(), 
        juce::RectanglePlacement::centred
    );
}

void SettingsComponent::resized() {
    auto bounds = getLocalBounds().reduced(CONTENT_INSET).withTrimmedTop(PADDING);

    const int titleLabelHeight = 16;
    auto titleBounds = bounds.removeFromTop(titleLabelHeight);
    title.setBounds(titleBounds);
    bounds.removeFromTop(PADDING);

    const int detailLabelHeight = 12;
    auto detailBounds = bounds.removeFromTop(detailLabelHeight);
    details.setBounds(detailBounds);
    bounds.removeFromTop(PADDING);

    juce::FlexBox controlColumn(juce::FlexBox::JustifyContent::center);
    controlColumn.flexDirection = juce::FlexBox::Direction::column;
    controlColumn.alignItems = juce::FlexBox::AlignItems::center;

    tooltipToggle.setLabelDimensions(66.0f, 12.0f);
    tooltipToggle.setControlDimensions(12.0f, 12.0f);
    tooltipToggle.setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f));
    tooltipToggle.setLabelPosition(LabelledControl<HoverableToggleButton>::LabelPosition::LEFT);
    controlColumn.items.add(juce::FlexItem(tooltipToggle)
        .withFlex(0.0f)
        .withWidth(static_cast<float>(bounds.getWidth()))
        .withHeight(40.0f)
        .withMargin(10.0f));

    std::vector<LabelledControl<juce::ComboBox>*> selectors = { &positionIndicatorSelector, &fieldIndicatorSelector, &controlRateSelector };

    for (auto* selector : selectors) {
        selector->setLabelDimensions(112.0f, 12.0f);
        selector->setControlDimensions(146.0f, 30.0f);
        selector->setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 0.0f, 10.0f));
        selector->setLabelPosition(LabelledControl<juce::ComboBox>::LabelPosition::LEFT);
        controlColumn.items.add(juce::FlexItem(*selector)
            .withFlex(0.0f)
            .withWidth(static_cast<float>(bounds.getWidth()))
            .withHeight(40.0f)
            .withMargin(10.0f));
    }

    controlColumn.performLayout(bounds.removeFromTop(CONTROL_COLUMN_HEIGHT));
}