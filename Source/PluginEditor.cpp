#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Utilities.h"

/* PRIVATE */

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    if (parameterID == "Position Pattern" || parameterID == "Position Mod A" || parameterID == "Position Mod B")
        positionPatternChanged.set(true);
    else if (parameterID == "Field Pattern" || parameterID == "Field Mod A" || parameterID == "Field Mod B")
        audioProcessor.updateField.store(true);
}

void MareverbAudioProcessorEditor::timerCallback() {
    if (positionPatternChanged.exchange(false))
        polarMapComponent.notifyPathChanged();
    if (audioProcessor.positionChanged.exchange(false, std::memory_order_acquire))
        polarMapComponent.notifyPositionChanged(audioProcessor.position.load(std::memory_order_relaxed));
    if (audioProcessor.fieldChanged.exchange(false, std::memory_order_acquire)) {
        std::vector<PolarCoordinate> coordinates;
        {
            std::lock_guard<std::mutex> lock(audioProcessor.fieldMutex);
            coordinates = audioProcessor.fieldCoordinates;
        }
        polarMapComponent.notifyFieldChanged(std::move(coordinates));
    }
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor (MareverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

    polarMapComponent(audioProcessor),

    globalMixControlAttachment(audioProcessor.apvts, "Global Mix", globalMixControl),
    decayControlAttachment(audioProcessor.apvts, "Decay", decayControl),
    positionPatternControlAttachment(audioProcessor.apvts, "Position Pattern", positionPatternControl),
    positionRateControlAttachment(audioProcessor.apvts, "Position Rate", positionRateControl),
    positionModAControlAttachment(audioProcessor.apvts, "Position Mod A", positionModAControl),
    positionModBControlAttachment(audioProcessor.apvts, "Position Mod B", positionModBControl),
    fieldPatternControlAttachment(audioProcessor.apvts, "Field Pattern", fieldPatternControl),
    fieldRateControlAttachment(audioProcessor.apvts, "Field Rate", fieldRateControl),
    fieldModAControlAttachment(audioProcessor.apvts, "Field Mod A", fieldModAControl),
    fieldModBControlAttachment(audioProcessor.apvts, "Field Mod B", fieldModBControl) {

    setSize(405, 621);

    // Attach listeners
    audioProcessor.apvts.addParameterListener("Global Mix", this);
    audioProcessor.apvts.addParameterListener("Decay", this);
    audioProcessor.apvts.addParameterListener("Position Pattern", this);
    audioProcessor.apvts.addParameterListener("Position Rate", this);
    audioProcessor.apvts.addParameterListener("Position Mod A", this);
    audioProcessor.apvts.addParameterListener("Position Mod B", this);
    audioProcessor.apvts.addParameterListener("Field Pattern", this);
    audioProcessor.apvts.addParameterListener("Field Rate", this);
    audioProcessor.apvts.addParameterListener("Field Mod A", this);
    audioProcessor.apvts.addParameterListener("Field Mod B", this);

    // Add components
    addAndMakeVisible(polarMapComponent);
    for (auto* component : getComponents()) addAndMakeVisible(component);

    // Control settings
    positionPatternControl.addItemList(positionPatterns, 1);
    fieldPatternControl.addItemList(fieldPatterns, 1);

    startTimerHz(REFRESH_RATE);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() { 
    // Detach listeners
    audioProcessor.apvts.removeParameterListener("Global Mix", this);
    audioProcessor.apvts.removeParameterListener("Decay", this);
    audioProcessor.apvts.removeParameterListener("Position Pattern", this);
    audioProcessor.apvts.removeParameterListener("Position Rate", this);
    audioProcessor.apvts.removeParameterListener("Position Mod A", this);
    audioProcessor.apvts.removeParameterListener("Position Mod B", this);
    audioProcessor.apvts.removeParameterListener("Field Pattern", this);
    audioProcessor.apvts.removeParameterListener("Field Rate", this);
    audioProcessor.apvts.removeParameterListener("Field Mod A", this);
    audioProcessor.apvts.removeParameterListener("Field Mod B", this);
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
    juce::FlexBox positionControlRow;
    std::vector<juce::Component*> positionControls = { &positionPatternControl, &positionRateControl, &positionModAControl, &positionModBControl };
    for (auto* component : positionControls) 
        positionControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox fieldControlRow;
    std::vector<juce::Component*> fieldControls = { &fieldPatternControl, &fieldRateControl, &fieldModAControl, &fieldModBControl };
    for (auto* component : fieldControls)
        fieldControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox globalControlRow;
    std::vector<juce::Component*> globalControls = { &globalMixControl, &decayControl };
    for (auto* component : globalControls) 
        globalControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    // Main parameter column
    juce::FlexBox uiColumn;
    uiColumn.flexDirection = juce::FlexBox::Direction::column;
    uiColumn.items.add(juce::FlexItem(positionControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(fieldControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(globalControlRow).withFlex(1.0f));
    uiColumn.performLayout(bounds);
}

std::vector<juce::Component*> MareverbAudioProcessorEditor::getComponents() {
    return { &globalMixControl, &decayControl, 
        &positionPatternControl, &positionRateControl, &positionModAControl, &positionModBControl,
        &fieldPatternControl, &fieldRateControl, &fieldModAControl, &fieldModBControl };
}