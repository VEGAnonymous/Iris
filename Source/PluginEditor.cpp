#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Utilities.h"

/* PRIVATE */

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (parameterID == "Motion Pattern" || parameterID == "Motion Mod A" || parameterID == "Motion Mod B") {
        motionPatternChanged.set(true);
    }
}
void MareverbAudioProcessorEditor::timerCallback() {
    if (motionPatternChanged.exchange(false))
        polarMapComponent.notifyPathChanged();
    if (audioProcessor.positionChanged.exchange(false, std::memory_order_acquire))
        polarMapComponent.notifyPositionChanged(audioProcessor.position.load(std::memory_order_relaxed));
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor (MareverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

    polarMapComponent(audioProcessor),

    globalMixControlAttachment(audioProcessor.apvts, "Global Mix", globalMixControl),
    decayControlAttachment(audioProcessor.apvts, "Decay", decayControl),
    motionPatternControlAttachment(audioProcessor.apvts, "Motion Pattern", motionPatternControl),
    motionRateControlAttachment(audioProcessor.apvts, "Motion Rate", motionRateControl),
    motionModAControlAttachment(audioProcessor.apvts, "Motion Mod A", motionModAControl),
    motionModBControlAttachment(audioProcessor.apvts, "Motion Mod B", motionModBControl) {

    setSize(405, 621);

    // Attach listeners
    audioProcessor.apvts.addParameterListener("Global Mix", this);
    audioProcessor.apvts.addParameterListener("Decay", this);
    audioProcessor.apvts.addParameterListener("Motion Pattern", this);
    audioProcessor.apvts.addParameterListener("Motion Rate", this);
    audioProcessor.apvts.addParameterListener("Motion Mod A", this);
    audioProcessor.apvts.addParameterListener("Motion Mod B", this);

    // Add components
    addAndMakeVisible(polarMapComponent);
    for (auto* component : getComponents()) addAndMakeVisible(component);

    // Control settings
    motionPatternControl.addItemList(motionPatterns, 1);
    
    startTimerHz(REFRESH_RATE);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() { 
    // Detach listeners
    audioProcessor.apvts.removeParameterListener("Global Mix", this);
    audioProcessor.apvts.removeParameterListener("Decay", this);
    audioProcessor.apvts.removeParameterListener("Motion Pattern", this);
    audioProcessor.apvts.removeParameterListener("Motion Rate", this);
    audioProcessor.apvts.removeParameterListener("Motion Mod A", this);
    audioProcessor.apvts.removeParameterListener("Motion Mod B", this);
}

// GUI

void MareverbAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
}

void MareverbAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    // Map
    auto mareMapArea = bounds.removeFromTop(405);
    polarMapComponent.setBounds(mareMapArea);

    // Parameter rows
    juce::FlexBox motionControlRow;
    std::vector<juce::Component*> motionControls = { &motionPatternControl, &motionRateControl, &motionModAControl, &motionModBControl };
    for (auto* component : motionControls) 
        motionControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox globalControlRow;
    std::vector<juce::Component*> globalControls = { &globalMixControl, &decayControl };
    for (auto* component : globalControls) 
        globalControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    // Main parameter column
    juce::FlexBox uiColumn;
    uiColumn.flexDirection = juce::FlexBox::Direction::column;
    uiColumn.items.add(juce::FlexItem(motionControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(globalControlRow).withFlex(1.0f));
    uiColumn.performLayout(bounds);
}

std::vector<juce::Component*> MareverbAudioProcessorEditor::getComponents() {
    return { &globalMixControl, &decayControl, &motionPatternControl, &motionRateControl, &motionModAControl, &motionModBControl };
}