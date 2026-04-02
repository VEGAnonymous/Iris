#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

class MareverbAudioProcessorEditor  : public juce::AudioProcessorEditor {
public:

    MareverbAudioProcessorEditor (MareverbAudioProcessor&);
    ~MareverbAudioProcessorEditor() override;
    void paint (juce::Graphics&) override;
    void resized() override;

private:

    // This reference is provided as a quick way for your editor to access the processor object that created it
    MareverbAudioProcessor& audioProcessor; 

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MareverbAudioProcessorEditor)
};
