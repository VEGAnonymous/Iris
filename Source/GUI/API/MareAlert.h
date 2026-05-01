#pragma once

#include "GUI/Components/Controls/HoverableTextButton.h"

#include <JuceHeader.h>

class MareAlert : public juce::Component {
private:
    juce::ComponentDragger dragger;

    juce::Label titleLabel, messageLabel, detailLabel;
    HoverableTextButton closeButton;

    static constexpr int 
        ALERT_WINDOW_WIDTH = 300, 
        PADDING = 16,
        ICON_HEIGHT = 180,
        BUTTON_HEIGHT = 40;

    static constexpr float 
        TITLE_FONT_HEIGHT = 24.0f, 
        MESSAGE_FONT_HEIGHT = 14.0f, 
        DETAIL_FONT_HEIGHT = 12.0f;

    float titleHeight = 0.0f, messageHeight = 0.0f, detailHeight = 0.0f;

    BoundsF iconBounds;

    inline static float getTextHeight(const juce::String& text, const juce::Font& font, float width);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MareAlert)

public:
    MareAlert(juce::AnimatorUpdater& updater, juce::LookAndFeel_V4* lookAndFeel, 
        const juce::String& title = "Mares!", const juce::String& message = "maremaremare", 
        const juce::String& details = "amreamreamre", const juce::String& buttonText = "I love them!");

    ~MareAlert() override;

    static void show(juce::AnimatorUpdater& updater, juce::LookAndFeel_V4* lookAndFeel,
        const juce::String& title = "Mares!", const juce::String& message = "maremaremare", 
        const juce::String& details = "amreamreamre", const juce::String & buttonText = "I love them!");

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;

    void paint(juce::Graphics&) override;
    void resized() override;
};