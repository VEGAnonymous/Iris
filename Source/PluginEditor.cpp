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
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load()) + 1, juce::dontSendNotification);

    fieldPatternControl.addItemList(fieldPatterns, 1);
    fieldPatternControl.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load()) + 1, juce::dontSendNotification);

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

    positionTabButton.setButtonText("POSITION");
    positionTabButton.onClick = selectPositionTab;

    fieldTabButton.setButtonText("FIELD");
    fieldTabButton.onClick = selectFieldTab;

    selectPositionTab();

    // Randomize / clear all buttons
    randomAllButton.setButtonText("Randomize IRs");
    randomAllButton.onClick = [this]() { audioProcessor.loadRandomIRs(); };

    clearAllButton.setButtonText("Clear IRs");
    clearAllButton.onClick = [this]() { audioProcessor.clearIRs(); };
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
    addAndMakeVisible(positionTabButton);
    addAndMakeVisible(fieldTabButton);
    addAndMakeVisible(randomAllButton);
    addAndMakeVisible(clearAllButton);
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

    Bounds totalBounds = getLocalBounds();

    g.setColour(juce::Colours::darkgrey.withAlpha(0.8f));

    // Left panel
    Bounds leftPanel = totalBounds.removeFromLeft(621);

    Bounds mapBounds = leftPanel.removeFromTop(621);
    Bounds& positionFieldControlsBounds = leftPanel;
    g.setColour(juce::Colours::darkgrey.withAlpha(0.2f));
    g.fillRect(positionFieldControlsBounds);

    // Right panel
    Bounds& rightPanel = totalBounds;

    Bounds topBarBounds = rightPanel.removeFromTop(81); // Randomize / Clear All, settings modal, Mareverb title
    g.setColour(juce::Colours::darkgrey.withAlpha(0.9f));
    g.fillRect(topBarBounds);

    Bounds irGridBounds = rightPanel.removeFromTop(160);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.8f));
    g.fillRect(irGridBounds);

    Bounds irHeaderBounds = rightPanel.removeFromTop(40);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.7f));
    g.fillRect(irHeaderBounds);
    
    Bounds irWaveformBounds = rightPanel.removeFromTop(140);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.6f));
    g.fillRect(irWaveformBounds);

    Bounds irControlsBounds = rightPanel.removeFromTop(100);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.5f));
    g.fillRect(irControlsBounds);

    Bounds interactionControlsBounds = rightPanel.removeFromTop(100);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.4f));
    g.fillRect(interactionControlsBounds);

    Bounds globalControlsBounds = rightPanel.removeFromTop(100);
    g.setColour(juce::Colours::darkgrey.withAlpha(0.3f));
    g.fillRect(globalControlsBounds);
}

void MareverbAudioProcessorEditor::resized() {
    auto fillFlex = [this](juce::FlexBox& flex, ControlGroup group) {
        for (const auto& control : controls)
            if (control.group == group)
                flex.items.add(juce::FlexItem(*control.component).withFlex(1.0f).withHeight(80).withMargin(10));
        };

    Bounds totalBounds = getLocalBounds();

    // Left panel
    Bounds leftPanel = totalBounds.removeFromLeft(621);

    Bounds mapBounds = leftPanel.removeFromTop(621);
    polarMapComponent.setBounds(mapBounds);

    Bounds& positionFieldControlsBounds = leftPanel; 

    juce::FlexBox positionFieldTabs;
    positionFieldTabs.flexDirection = juce::FlexBox::Direction::column;
    positionFieldTabs.items.add(juce::FlexItem(positionTabButton).withFlex(1.0f));
    positionFieldTabs.items.add(juce::FlexItem(fieldTabButton).withFlex(1.0f));
    positionFieldTabs.performLayout(positionFieldControlsBounds.removeFromLeft(static_cast<int>(positionFieldControlsBounds.getWidth() * 0.15f)));

    juce::FlexBox positionControlRow, fieldControlRow;
    fillFlex(positionControlRow, ControlGroup::POSITION);
    fillFlex(fieldControlRow, ControlGroup::FIELD);
    positionControlRow.performLayout(positionFieldControlsBounds);
    fieldControlRow.performLayout(positionFieldControlsBounds);

    // Right panel
    Bounds& rightPanel = totalBounds;

    Bounds topBarBounds = rightPanel.removeFromTop(81); // Randomize / Clear All, settings modal, Mareverb title
    juce::FlexBox globalIROperations;
    globalIROperations.flexDirection = juce::FlexBox::Direction::column;
    globalIROperations.items.add(juce::FlexItem(randomAllButton).withFlex(1.0f));
    globalIROperations.items.add(juce::FlexItem(clearAllButton).withFlex(1.0f));
    globalIROperations.performLayout(topBarBounds.removeFromLeft(static_cast<int>(topBarBounds.getWidth() * 0.25f)));

    Bounds irGridBounds = rightPanel.removeFromTop(160);

    Bounds irHeaderBounds = rightPanel.removeFromTop(40); // IR (indicator color) - filename, Clear button

    Bounds irWaveformBounds = rightPanel.removeFromTop(140);

    Bounds irControlsBounds = rightPanel.removeFromTop(100); // Load (Manual) button, Load (Random) button, Auto Min textbox, Auto Max textbox

    Bounds interactionControlsBounds = rightPanel.removeFromTop(100); // Weighting Mode, Strength, Spread

    juce::FlexBox interactionControlRow;
    fillFlex(interactionControlRow, ControlGroup::INTERACTION);
    interactionControlRow.performLayout(interactionControlsBounds);

    Bounds globalControlsBounds = rightPanel.removeFromTop(100); // Low Cut, High Cut, Decay, Mix

    juce::FlexBox globalControlRow;
    fillFlex(globalControlRow, ControlGroup::GLOBAL);
    globalControlRow.performLayout(globalControlsBounds.removeFromLeft(static_cast<int>(globalControlsBounds.getWidth() * 0.8f)));

}

std::vector<ControlDef> MareverbAudioProcessorEditor::getControls() { return controls; }

std::vector<ControlDef> MareverbAudioProcessorEditor::getControlsGroup(ControlGroup group) {
    std::vector<ControlDef> groupControls;
    for (const auto& control : controls)
        if (control.group == group)
            groupControls.push_back(control);
    return groupControls;
}