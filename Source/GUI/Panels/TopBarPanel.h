#pragma once

#include "GUI/Components/SettingsComponent.h"
#include "GUI/Components/Controls/HoverableImageButton.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class TopBarPanel : public juce::Component {
private:
    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    HoverableTextButton clearAllButton {animatorUpdater}, randomAllButton {animatorUpdater};
    HoverableImageButton directoryManagerButton {animatorUpdater}, settingsButton {animatorUpdater};

    HoverableImageButton logo {animatorUpdater};

    juce::Random sourceRNG;
    juce::StringArray sources = {
        "https://derpicdn.net/img/view/2021/10/31/2735776.gif",
        "https://derpicdn.net/img/view/2022/5/11/2863256.gif",
        "https://derpicdn.net/img/view/2018/1/5/1624875.gif",
        "https://derpicdn.net/img/view/2014/10/14/743094.gif"
    };

    void prepare();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(TopBarPanel)

public:
    std::function<void()> onDirectoryManagerClicked;
    std::function<void()> onSettingsClicked;

    TopBarPanel(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~TopBarPanel() override = default;

    void resized() override;
    void paint(juce::Graphics& g) override;
};