#include "PluginProcessor.h"
#include "PluginEditor.h"

/* PRIVATE */

void MareverbAudioProcessorEditor::parameterValueChanged(int parameterIndex, float newValue) { parametersChanged.set(true); }
void MareverbAudioProcessorEditor::timerCallback() {
    if (parametersChanged.compareAndSetBool(false, true)) {
        // Update internal GUI state according to parameters
        // Repaint
    }
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor (MareverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p), 
    globalMixControlAttachment(audioProcessor.apvts, "Global Mix", globalMixControl),
    decayControlAttachment(audioProcessor.apvts, "Decay", decayControl),
    motionRateControlAttachment(audioProcessor.apvts, "Motion Rate", motionRateControl),
    motionModAControlAttachment(audioProcessor.apvts, "Motion Mod A", motionModAControl),
    motionModBControlAttachment(audioProcessor.apvts, "Motion Mod B", motionModBControl)
{
    for (auto* component : getComponents()) addAndMakeVisible(component);

    const auto& params = audioProcessor.getParameters();
    for (auto param : params) param->addListener(this);

    startTimerHz(REFRESH_RATE);
    setSize (405, 621);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() { 
    const auto& params = audioProcessor.getParameters();
    for (auto param : params) param->removeListener(this);
}

// GUI

void MareverbAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    auto bounds = getLocalBounds();
    auto mareMapArea = bounds.removeFromTop(405);

    auto w = mareMapArea.getWidth();

    g.setColour(juce::Colours::floralwhite);
    g.drawRoundedRectangle(mareMapArea.toFloat(), 4.0f, 1.0f);
    g.drawEllipse(mareMapArea.toFloat(), 2.0f);


}

void MareverbAudioProcessorEditor::resized() {
    // Window layout
    auto bounds = getLocalBounds();

    // Map
    auto mareMapArea = bounds.removeFromTop(405);

    // Parameter rows
    juce::FlexBox motionControlRow;
    std::vector<juce::Component*> motionControls = { &motionRateControl, &motionModAControl, &motionModBControl };
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
    return { &globalMixControl, &decayControl, &motionRateControl, &motionModAControl, &motionModBControl };
}