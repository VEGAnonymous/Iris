#pragma once

#include "Core/Control/IRManager.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Theme/Theme.h"
#include "GUI/Theme/LookAndFeel/ButtonLookAndFeel.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class DirectoryManagerComponent : public juce::Component, public juce::ListBoxModel {
private:
    static constexpr auto TITLE_ROW_HEIGHT = 36,
                          BUTTON_COLUMN_WIDTH = 20,
                          BUTTON_COLUMN_PADDING = 12,
                          DIRECTORY_ROW_HEIGHT = 28,
                          DIRECTORY_LIST_HEIGHT = 240,
                          CONTROL_COLUMN_HEIGHT = 40;

    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    juce::Label title;
    juce::ListBox directoryList { {}, this };

    HoverableTextButton addButton, removeButton;

    LabelledControl<juce::ComboBox> randomModeSelector { "Random Sampling Method" };
    const juce::StringArray randomModes { "Uniform across all files", "Uniform across directories" };

    int selectedRow = -1;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectoryManagerComponent)

public:
    class DirectoryRowComponent : public juce::Component {
    private:
        HoverableToggleButton activeToggle;
        juce::Label pathLabel;

    public:
        std::function<void(bool)> onToggle;

        DirectoryRowComponent(juce::AnimatorUpdater& updater) : activeToggle(updater) {
            addAndMakeVisible(activeToggle);
            addAndMakeVisible(pathLabel);
            pathLabel.setMinimumHorizontalScale(1.0f);
            pathLabel.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(13.0f).withKerningFactor(0.01f)));

            this->setInterceptsMouseClicks(false, true);
            pathLabel.setInterceptsMouseClicks(false, false);

            activeToggle.onClick = [this] {
                if (onToggle) onToggle(activeToggle.getToggleState());
            };
        }

        void update(const juce::String& path, bool active) {
            pathLabel.setText(path, juce::dontSendNotification);
            pathLabel.setColour(juce::Label::ColourIds::textColourId, 
                (Theme::Colors::textLight).withMultipliedAlpha(active ? 1.0f : 0.5f));
            activeToggle.setToggleState(active, juce::dontSendNotification);
        }

        void resized() override {
            auto bounds = getLocalBounds();
            activeToggle.setBounds(bounds.removeFromLeft(24).reduced(8));
            pathLabel.setBounds(bounds);
        }
    };

    DirectoryManagerComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~DirectoryManagerComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    void refresh();
};