#pragma once

#include "ButtonLookAndFeel.h"
#include "HoverableToggleButton.h"
#include "HoverableTextButton.h"
#include "IRManager.h"
#include "Theme.h"

#include <JuceHeader.h>

class DirectoryManagerComponent : public juce::Component, public juce::ListBoxModel {
private:
    IRManager* irManager;
    juce::AnimatorUpdater& animatorUpdater;

    juce::Label title;
    juce::ListBox directoryList { {}, this };
    HoverableTextButton addButton, removeButton;

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
            pathLabel.setFont(Theme::Fonts::getEquestriaNeueFont(juce::FontOptions(13.0f)));

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

    DirectoryManagerComponent(IRManager* irManager, juce::AnimatorUpdater& updater);
    ~DirectoryManagerComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

    int getNumRows() override;
    void paintListBoxItem(int rowNumber, juce::Graphics& g, int width, int height, bool rowIsSelected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent&) override;
    juce::Component* refreshComponentForRow(int rowNumber, bool isRowSelected, juce::Component* existingComponentToUpdate) override;

    void refresh();
};