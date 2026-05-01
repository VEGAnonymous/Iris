#include "GUI/API/MareAlert.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

float MareAlert::getTextHeight(const juce::String& text, const juce::Font& font, float width) {
    juce::AttributedString string;
    string.append(text, font);

    juce::TextLayout layout;
    layout.createLayout(string, width);

    return layout.getHeight();
}

/* PUBLIC */

MareAlert::MareAlert(juce::AnimatorUpdater& updater, juce::LookAndFeel_V4* lookAndFeel,
    const juce::String& title, const juce::String& message, const juce::String& details, const juce::String& buttonText) 
    : closeButton(updater) {

    setLookAndFeel(lookAndFeel);

    titleLabel.setText(title, juce::dontSendNotification);
    titleLabel.setFont(Theme::Fonts::getEquestriaBoldFont(juce::FontOptions(TITLE_FONT_HEIGHT)));
    titleLabel.setColour(juce::Label::textColourId, Theme::Colors::highlight);
    titleLabel.setJustificationType(juce::Justification::centred);
    titleLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(titleLabel);

    messageLabel.setText(message, juce::dontSendNotification);
    messageLabel.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(MESSAGE_FONT_HEIGHT)));
    messageLabel.setColour(juce::Label::textColourId, Theme::Colors::highlight);
    messageLabel.setJustificationType(juce::Justification::centred);
    messageLabel.setMinimumHorizontalScale(1.0f);
    messageLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(messageLabel);

    detailLabel.setText(details, juce::dontSendNotification);
    detailLabel.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(DETAIL_FONT_HEIGHT)));
    detailLabel.setColour(juce::Label::textColourId, Theme::Colors::textLight);
    detailLabel.setJustificationType(juce::Justification::centred);
    detailLabel.setMinimumHorizontalScale(1.0f);
    detailLabel.setInterceptsMouseClicks(false, false);
    addAndMakeVisible(detailLabel);

    closeButton.setButtonText(buttonText);
    closeButton.onClick = [this]() {
        if (auto* modalManager = juce::ModalComponentManager::getInstance())
            modalManager->cancelAllModalComponents();
        juce::MessageManager::callAsync([this] { delete this; });
    };
    closeButton.setInterceptsMouseClicks(true, true);
    addAndMakeVisible(closeButton);

    float contentWidth = static_cast<float>(ALERT_WINDOW_WIDTH - (PADDING * 2));
    titleHeight = getTextHeight(titleLabel.getText(), titleLabel.getFont(), contentWidth);
    messageHeight = getTextHeight(messageLabel.getText(), messageLabel.getFont(), contentWidth);
    detailHeight = getTextHeight(detailLabel.getText(), detailLabel.getFont(), contentWidth);

    int alertWindowHeight = static_cast<int>(
        static_cast<float>(PADDING * 2) 
        + titleHeight + messageHeight + detailHeight + static_cast<float>(ICON_HEIGHT) + static_cast<float>(BUTTON_HEIGHT)
        + static_cast<float>(PADDING * 5));

    setSize(ALERT_WINDOW_WIDTH, alertWindowHeight);
}

MareAlert::~MareAlert() {
    setLookAndFeel(nullptr);
}

void MareAlert::mouseDown(const juce::MouseEvent& e) { dragger.startDraggingComponent(this, e); }
void MareAlert::mouseDrag(const juce::MouseEvent& e) { dragger.dragComponent(this, e, nullptr); }

void MareAlert::paint(juce::Graphics& g) {
    auto bounds = getLocalBounds().toFloat();
    const float cornerSize = 12.0f;

    g.setColour(Theme::Colors::outline);
    g.drawRoundedRectangle(bounds, cornerSize, 4.0f);

    g.setColour(Theme::Colors::background);
    g.fillRoundedRectangle(bounds.reduced(1.0f), cornerSize);

    juce::Image icon = Theme::Images::getDerpyHooves();
    jassert(!icon.isNull());
    if (!icon.isNull() && !iconBounds.isEmpty())
        g.drawImage(icon, iconBounds, juce::RectanglePlacement::centred);
}

void MareAlert::resized() {
    auto bounds = getLocalBounds().reduced(PADDING);

    titleLabel.setBounds(bounds.removeFromTop(static_cast<int>(titleHeight)));
    bounds.removeFromTop(PADDING);

    messageLabel.setBounds(bounds.removeFromTop(static_cast<int>(messageHeight)));
    bounds.removeFromTop(PADDING);

    detailLabel.setBounds(bounds.removeFromTop(static_cast<int>(detailHeight)));
    bounds.removeFromTop(PADDING * 2);

    iconBounds = bounds.removeFromTop(ICON_HEIGHT).toFloat();

    bounds.removeFromTop(PADDING);
    closeButton.setBounds(bounds.removeFromTop(BUTTON_HEIGHT)
        .withSizeKeepingCentre(static_cast<int>(ALERT_WINDOW_WIDTH * 0.261f), BUTTON_HEIGHT));
}

void MareAlert::show(juce::AnimatorUpdater& updater, juce::LookAndFeel_V4* lookAndFeel,
    const juce::String& title, const juce::String& message, const juce::String& details, const juce::String& buttonText) {

    auto* alert = new MareAlert(updater, lookAndFeel, title, message, details, buttonText);

    if (auto* topLevelComponent = juce::Desktop::getInstance().getComponent(0))
        alert->setCentrePosition(topLevelComponent->getBounds().getCentre());

    alert->addToDesktop(juce::ComponentPeer::windowIsTemporary | juce::ComponentPeer::windowHasTitleBar * 0);
    alert->setVisible(true);
    alert->enterModalState(true, nullptr, false);
}