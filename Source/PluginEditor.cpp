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

void MareverbAudioProcessorEditor::initComponents() {
    // Weighting mode toggle switch
    weightingModeControl.setClickingTogglesState(true);
    auto updateWeightingModeText = [this] { weightingModeControl.setButtonText(weightingModes[weightingModeControl.getToggleState()]); };
    updateWeightingModeText();
    weightingModeControl.onStateChange = updateWeightingModeText;

    // Pattern combo boxes
    positionPatternControl.addItemList(positionPatterns, 1);
    positionPatternControl.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load()) + 1, true);

    fieldPatternControl.addItemList(fieldPatterns, 1);
    fieldPatternControl.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load()) + 1, true);

    // Position / field tab selector
    // HACK: Don't repeat yourself
    auto positionControls = getControlsGroup(ControlGroup::POSITION);
    auto fieldControls = getControlsGroup(ControlGroup::FIELD);

    auto selectPositionTab = [this, positionControls, fieldControls] {
        for (auto& positionControl : positionControls) positionControl.component->setVisible(true);
        for (auto& fieldControl : fieldControls) fieldControl.component->setVisible(false); 
    };

    auto selectFieldTab = [this, positionControls, fieldControls] {
        for (auto& positionControl : positionControls) positionControl.component->setVisible(false);
        for (auto& fieldControl : fieldControls) fieldControl.component->setVisible(true);
    };

    positionTabSelect.setButtonText("POSITION");
    positionTabSelect.onClick = selectPositionTab;

    fieldTabSelect.setButtonText("FIELD");
    fieldTabSelect.onClick = selectFieldTab;

    selectPositionTab();
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor (MareverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

    polarMapComponent(audioProcessor),

    globalMixControlAttachment(audioProcessor.apvts, ParamID::globalMix, globalMixControl),
    decayControlAttachment(audioProcessor.apvts, ParamID::decay, decayControl),
    lowCutControlAttachment(audioProcessor.apvts, ParamID::lowCut, lowCutControl),
    highCutControlAttachment(audioProcessor.apvts, ParamID::highCut, highCutControl),

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

    setSize(1046, 721);

    // Attach listeners
    for (const auto& id : paramIDs) audioProcessor.apvts.addParameterListener(id, this);

    // Add components
    addAndMakeVisible(polarMapComponent);
    addAndMakeVisible(positionTabSelect);
    addAndMakeVisible(fieldTabSelect);
    for (auto& control : getControls()) addAndMakeVisible(control.component);

    // Setup components
    initComponents();

    startTimerHz(REFRESH_RATE);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() { 
    // Detach listeners
    for (const auto& id : paramIDs) audioProcessor.apvts.removeParameterListener(id, this);
}

// GUI

void MareverbAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    auto bounds = getLocalBounds();

    g.setColour(juce::Colours::darkgrey.withAlpha(0.8f));

    // Left panel
    auto leftPanel = bounds.removeFromLeft(621);

    auto mapArea = leftPanel.removeFromTop(621);
    auto& positionFieldControlsArea = leftPanel;
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    g.fillRect(positionFieldControlsArea);

    // Right panel
    auto& rightPanel = bounds;

    auto topBarArea = rightPanel.removeFromTop(81); // Randomize / Clear All, settings modal, Mareverb title
    g.setColour(juce::Colours::darkgrey.withAlpha(0.9f));
    g.fillRect(topBarArea);

    auto irGridArea = rightPanel.removeFromTop(160);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.8f));
    g.fillRect(irGridArea);

    auto irHeaderArea = rightPanel.removeFromTop(40);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.7f));
    g.fillRect(irHeaderArea);
    
    auto irWaveformArea = rightPanel.removeFromTop(140);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.6f));
    g.fillRect(irWaveformArea);

    auto irControlsArea = rightPanel.removeFromTop(100);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    g.fillRect(irControlsArea);

    auto interactionControlsArea = rightPanel.removeFromTop(100);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.4f));
    g.fillRect(interactionControlsArea);

    auto globalControlsArea = rightPanel.removeFromTop(100);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    g.fillRect(globalControlsArea);
}

void MareverbAudioProcessorEditor::resized() {
    auto controls = getControls();
    auto fillFlex = [controls](juce::FlexBox& flex, ControlGroup group) {
        for (const auto& control : controls)
            if (control.group == group)
                flex.items.add(juce::FlexItem(*control.component).withFlex(1.0f).withHeight(80).withMargin(10));
        };

    auto bounds = getLocalBounds();

    // Left panel
    auto leftPanel = bounds.removeFromLeft(621);

    auto mapArea = leftPanel.removeFromTop(621);
    polarMapComponent.setBounds(mapArea);

    auto& positionFieldControlsArea = leftPanel;

    juce::FlexBox positionFieldTabSelector;
    positionFieldTabSelector.flexDirection = juce::FlexBox::Direction::column;
    positionFieldTabSelector.items.add(juce::FlexItem(positionTabSelect).withFlex(1.0f));
    positionFieldTabSelector.items.add(juce::FlexItem(fieldTabSelect).withFlex(1.0f));
    positionFieldTabSelector.performLayout(positionFieldControlsArea.removeFromLeft(static_cast<int>(positionFieldControlsArea.getWidth() * 0.15f)));

    juce::FlexBox positionControlRow, fieldControlRow;
    fillFlex(positionControlRow, ControlGroup::POSITION);
    fillFlex(fieldControlRow, ControlGroup::FIELD);
    positionControlRow.performLayout(positionFieldControlsArea);
    fieldControlRow.performLayout(positionFieldControlsArea);

    // Right panel
    auto& rightPanel = bounds;

    auto topBarArea = rightPanel.removeFromTop(81); // Randomize / Clear All, settings modal, Mareverb title

    auto irGridArea = rightPanel.removeFromTop(160);

    auto irHeaderArea = rightPanel.removeFromTop(40); // IR (indicator color) - filename, Clear button

    auto irWaveformArea = rightPanel.removeFromTop(140);

    auto irControlsArea = rightPanel.removeFromTop(100); // Load (Manual) button, Load (Random) button, Auto Min textbox, Auto Max textbox

    auto interactionControlsArea = rightPanel.removeFromTop(100); // Weighting Mode, Strength, Spread

    juce::FlexBox interactionControlRow;
    fillFlex(interactionControlRow, ControlGroup::INTERACTION);
    interactionControlRow.performLayout(interactionControlsArea);

    auto globalControlsArea = rightPanel.removeFromTop(100); // Low Cut, High Cut, Decay, Mix

    juce::FlexBox globalControlRow;
    fillFlex(globalControlRow, ControlGroup::GLOBAL);
    globalControlRow.performLayout(globalControlsArea.removeFromLeft(static_cast<int>(globalControlsArea.getWidth() * 0.8f)));

}

std::vector<ControlDef> MareverbAudioProcessorEditor::getControls() { return controls; }

std::vector<ControlDef> MareverbAudioProcessorEditor::getControlsGroup(ControlGroup group) {
    std::vector<ControlDef> groupControls;
    for (const auto& control : controls)
        if (control.group == group)
            groupControls.push_back(control);
    return groupControls;
}