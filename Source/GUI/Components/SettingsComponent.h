#pragma once

#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Theme/Theme.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class SettingsComponent : public juce::Component {
private:
    static constexpr auto
        CONTENT_INSET = 16,
        PADDING = 16,
        CONTROL_COLUMN_HEIGHT = 160;

    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    juce::Label title, details;

    LabelledControl<HoverableToggleButton> tooltipToggle {"Tooltips", animatorUpdater};

    LabelledControl<juce::ComboBox> positionIndicatorSelector { "Position Indicator" };
    const juce::StringArray positionIndicatorStyles { 
        PositionIndicatorStyle::Mareless, 
        PositionIndicatorStyle::Anon, 
        PositionIndicatorStyle::Aryanne,
        PositionIndicatorStyle::Fausticorn,
        PositionIndicatorStyle::Milky_Way,
        PositionIndicatorStyle::Nasapone,
        PositionIndicatorStyle::Sergeant_Reckless,
        PositionIndicatorStyle::Tracy_Cage, 
        PositionIndicatorStyle::Verity
    };

    LabelledControl<juce::ComboBox> fieldIndicatorSelector { "Field Indicators" };
    const juce::StringArray fieldIndicatorStyles { 
        FieldIndicatorStyle::Mareless, 
        FieldIndicatorStyle::Half_Mared, 
        FieldIndicatorStyle::Mareful 
    };

    LabelledControl<juce::ComboBox> controlRateSelector { "Control Rate" };

    juce::Image lyraImage;
    juce::Random lyraRNG;

    void prepare();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)

public:
    std::function<void()> onCloseRequested;

    SettingsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~SettingsComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;
};