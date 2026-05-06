#pragma once

#include "Core/Control/IRManager.h"
#include "GUI/Components/Controls/HoverableImageButton.h"
#include "GUI/Components/Controls/HoverableToggleButton.h"
#include "GUI/Components/Controls/HoverableTextButton.h"
#include "GUI/Components/Controls/HoverableTextEditor.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Theme/Theme.h"
#include "PluginProcessor.h"

#include <JuceHeader.h>

class DirectoryManagerComponent : public juce::Component, public juce::ListBoxModel {
private:
    static constexpr auto 
        CONTENT_INSET = 24,
        CONTENT_TOP_TRIM = 14,
        TITLE_ROW_HEIGHT = 40,
        BUTTON_COLUMN_WIDTH = 20,
        BUTTON_COLUMN_PADDING = 12,
        DIRECTORY_ROW_HEIGHT = 28,
        DIRECTORY_LIST_HEIGHT = 240,
        CONTROL_COLUMN_HEIGHT = 120;

    MareverbAudioProcessor& audioProcessor;
    juce::AnimatorUpdater& animatorUpdater;

    juce::Label title;
    juce::ListBox directoryList { {}, this };
    int selectedRow = -1;

    HoverableTextButton addButton, removeButton;
    HoverableImageButton refreshButton;

    LabelledControl<HoverableTextEditor> 
        fileFilterEditor { "File Filter", animatorUpdater },
        directoryFilterEditor { "Directory Filter", animatorUpdater };
    bool fileFilterChanged = false, directoryFilterChanged = false;

    LabelledControl<juce::ComboBox> samplingModeSelector { "Random Sampling Method" };
    const juce::StringArray samplingModes { "Uniform across all files", "Uniform across directories" };

    void prepare();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DirectoryManagerComponent)

public:
    class DirectoryRowComponent : public juce::Component {
    private:
        std::atomic<bool>& isBusy;
        HoverableToggleButton activeToggle;
        juce::Label pathLabel;

    public:
        std::function<void(bool)> onToggle;

        DirectoryRowComponent(juce::AnimatorUpdater& updater, std::atomic<bool>& busy) : activeToggle(updater), isBusy(busy) {
            activeToggle.setClickingTogglesState(false);
            activeToggle.setTooltip("Enables this directory.\nDisabled directories will not be drawn from when loading random IRs.");
            addAndMakeVisible(activeToggle);

            pathLabel.setMinimumHorizontalScale(1.0f);
            pathLabel.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(13.0f).withKerningFactor(0.01f)));
            addAndMakeVisible(pathLabel);

            this->setInterceptsMouseClicks(false, true);
            pathLabel.setInterceptsMouseClicks(false, false);

            activeToggle.onClick = [this] {
                if (isBusy.load(std::memory_order_acquire) || !onToggle) return;

                const bool nState = !activeToggle.getToggleState();
                activeToggle.setToggleState(nState, juce::dontSendNotification);
                onToggle(nState);
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

    std::function<void()> onCloseRequested;

    DirectoryManagerComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater);
    ~DirectoryManagerComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    void refresh();
};