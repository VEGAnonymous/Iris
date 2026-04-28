#pragma once

#include "GUI/Components/SettingsComponent.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class TopBarPanel : public juce::Component {
private:
    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    HoverableTextButton clearAllButton {animatorUpdater}, randomAllButton {animatorUpdater};
    HoverableTextButton settingsButton {animatorUpdater};

    void prepare();

public:
    std::function<void()> onSettingsClicked;

    TopBarPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~TopBarPanel() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;
};