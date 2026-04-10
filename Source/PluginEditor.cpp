#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Utilities.h"

/* PRIVATE */

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    if (parameterID == ParamID::positionPattern || parameterID == ParamID::positionModA || parameterID == ParamID::positionModB)
        positionPathChanged.store(true);
    else if (parameterID == ParamID::fieldPattern || parameterID == ParamID::fieldModA || parameterID == ParamID::fieldModB)
        audioProcessor.updateField.store(true);
}

void MareverbAudioProcessorEditor::timerCallback() {
    if (positionPathChanged.exchange(false, std::memory_order_acquire))
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

    globalMixControlAttachment(audioProcessor.apvts, ParamID::globalMix, globalMixControl),
    decayControlAttachment(audioProcessor.apvts, ParamID::decay, decayControl),

    weightingModeControlAttachment(audioProcessor.apvts, ParamID::weightingMode, weightingModeControl),
    strengthControlAttachment(audioProcessor.apvts, ParamID::strength, strengthControl),
    spreadControlAttachment(audioProcessor.apvts, ParamID::spread, spreadControl),

    positionPatternControlAttachment(audioProcessor.apvts, ParamID::positionPattern, positionPatternControl),
    positionRateControlAttachment(audioProcessor.apvts, ParamID::positionRate, positionRateControl),
    positionModAControlAttachment(audioProcessor.apvts, ParamID::positionModA, positionModAControl),
    positionModBControlAttachment(audioProcessor.apvts, ParamID::positionModB, positionModBControl),

    fieldPatternControlAttachment(audioProcessor.apvts, ParamID::fieldPattern, fieldPatternControl),
    fieldRateControlAttachment(audioProcessor.apvts, ParamID::fieldRate, fieldRateControl),
    fieldModAControlAttachment(audioProcessor.apvts, ParamID::fieldModA, fieldModAControl),
    fieldModBControlAttachment(audioProcessor.apvts, ParamID::fieldModB, fieldModBControl) {

    setSize(1216, 621);

    // Attach listeners
    for (const auto& id : paramIDs) audioProcessor.apvts.addParameterListener(id, this);

    // Add components
    addAndMakeVisible(polarMapComponent);
    for (auto* control : getControls()) addAndMakeVisible(control);

    // Control settings
    positionPatternControl.addItemList(positionPatterns, 1);
    fieldPatternControl.addItemList(fieldPatterns, 1);

    startTimerHz(REFRESH_RATE);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() { 
    // Detach listeners
    for (const auto& id : paramIDs) audioProcessor.apvts.removeParameterListener(id, this);
}

// GUI

void MareverbAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::black);
}

void MareverbAudioProcessorEditor::resized() {
    auto bounds = getLocalBounds();

    // Map
    auto mareMapArea = bounds.removeFromLeft(621);
    polarMapComponent.setBounds(mareMapArea);

    // Parameter rows
    juce::FlexBox positionControlRow;
    std::vector<juce::Component*> positionControls = { &positionPatternControl, &positionRateControl, &positionModAControl, &positionModBControl };
    for (auto* control : positionControls) 
        positionControlRow.items.add(juce::FlexItem(*control).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox fieldControlRow;
    std::vector<juce::Component*> fieldControls = { &fieldPatternControl, &fieldRateControl, &fieldModAControl, &fieldModBControl };
    for (auto* control : fieldControls)
        fieldControlRow.items.add(juce::FlexItem(*control).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox interactionControlRow;
    std::vector<juce::Component*> interactionControls = { &weightingModeControl, &strengthControl, &spreadControl };
    for (auto* control : interactionControls)
        interactionControlRow.items.add(juce::FlexItem(*control).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox globalControlRow;
    std::vector<juce::Component*> globalControls = { &globalMixControl, &decayControl };
    for (auto* control : globalControls) 
        globalControlRow.items.add(juce::FlexItem(*control).withFlex(1.0f).withMaxHeight(90));

    // Main parameter column
    juce::FlexBox uiColumn;
    uiColumn.flexDirection = juce::FlexBox::Direction::column;
    uiColumn.items.add(juce::FlexItem(positionControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(fieldControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(interactionControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(globalControlRow).withFlex(1.0f));
    uiColumn.performLayout(bounds);
}

std::vector<juce::Component*> MareverbAudioProcessorEditor::getControls() {
    return { 
        &globalMixControl, &decayControl, 
        &weightingModeControl, &strengthControl, &spreadControl,
        &positionPatternControl, &positionRateControl, &positionModAControl, &positionModBControl,
        &fieldPatternControl, &fieldRateControl, &fieldModAControl, &fieldModBControl 
    };
}