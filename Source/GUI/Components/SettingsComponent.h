#pragma once

#include "GUI/Components/DirectoryManagerComponent.h"
#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"

#include <JuceHeader.h>

class SettingsComponent : public juce::Component {
private:
    HoverableTextButton closeButton;
    DirectoryManagerComponent directoryManager;

    static constexpr auto SETTINGS_INSET = 16;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SettingsComponent)

public:
    SettingsComponent(IRManager* irManager, juce::AnimatorUpdater& updater);
    ~SettingsComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void refreshDirectories();

    std::function<void()> onCloseRequested;
};