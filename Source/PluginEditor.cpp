#include "GUIUtilities.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Utilities.h"

/* PRIVATE */

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    if (parameterID == ParamID::positionPattern || parameterID == ParamID::positionModA || parameterID == ParamID::positionModB) {
        positionPathChanged.store(true, std::memory_order_release);
        audioProcessor.guiState.updatePosition.store(true, std::memory_order_release);
        if (parameterID == ParamID::positionPattern)
            audioProcessor.guiState.syncingPosition.store(true, std::memory_order_release);
    } 
    
    if (parameterID == ParamID::fieldPattern || parameterID == ParamID::fieldModA || parameterID == ParamID::fieldModB) {
        audioProcessor.guiState.updateField.store(true, std::memory_order_release);
        if (parameterID == ParamID::fieldPattern)
            audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
    }
}

void MareverbAudioProcessorEditor::timerCallback() {
    if (positionPathChanged.exchange(false, std::memory_order_acquire))
        polarMapComponent.notifyPathChanged();

    if (audioProcessor.guiState.positionChanged.exchange(false, std::memory_order_acquire))
        polarMapComponent.notifyPositionChanged(audioProcessor.guiState.position.load(std::memory_order_relaxed));

    if (audioProcessor.guiState.fieldChanged.exchange(false, std::memory_order_acquire)) {
        std::vector<PolarCoordinate> coordinates;
        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.fieldLock);
            coordinates = audioProcessor.guiState.fieldCoordinates;
        }
        polarMapComponent.notifyFieldChanged(std::move(coordinates));
        // DBG("Passed coordinates to map");
    }

    auto updateIRSlot = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        // DBG("Selected IR " << selectedIR);
        if (validateIRIndex(selectedIR)) {
            const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
            irHeaderComponent.setSlot(selectedIR, slot);

            irWaveformComponent.setWaveform(&slot.buffer, WAVEFORM_POINTS);
            irWaveformComponent.setActive(slot.active);
            
            for (int i = 0; i < MAX_IR_COUNT; ++i) {
                if (swapControls[i]) {
                    swapControls[i]->swapMinControl.setVisible(i == selectedIR);
                    swapControls[i]->swapMaxControl.setVisible(i == selectedIR);
                }
            }
        }
    };

    if (audioProcessor.guiState.irChanged.exchange(false, std::memory_order_acquire)) {
        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
            irSlotButtons[i]->setOccupied(slot.occupied);
            irSlotButtons[i]->setActive(slot.active);
            irSlotButtons[i]->setWaveform(slot.occupied ? &slot.buffer : nullptr);
        }

        updateIRSlot();
    }

    if (selectedIRChanged.exchange(false, std::memory_order_acquire))
        updateIRSlot();

    if (audioProcessor.guiState.syncingPosition.load(std::memory_order_acquire)) {
        auto positionPattern = static_cast<PositionPattern>(audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load());
        auto* positionRate = audioProcessor.apvts.getParameter(ParamID::positionRate);
        auto* positionModA = audioProcessor.apvts.getParameter(ParamID::positionModA);
        auto* positionModB = audioProcessor.apvts.getParameter(ParamID::positionModB);

        PatternParameterState nMods;

        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.patternState.patternStateLock);
            audioProcessor.patternState.positionParamStates[audioProcessor.patternState.lastPositionPattern] =
            { positionRate->getValue(),
              positionModA->getValue(),
              positionModB->getValue() };

            nMods = audioProcessor.patternState.positionParamStates[positionPattern];
        }
        
        float nRate = nMods.rate,
              nModA = nMods.modA,
              nModB = nMods.modB;

        if (positionPattern == PositionPattern::MANUAL) {
            PolarCoordinate position = audioProcessor.guiState.position.load();
            nModA = positionModA->convertTo0to1(position.r);
            nModB = positionModB->convertTo0to1(position.theta / juce::MathConstants<float>::twoPi);
        }

        if (positionRate) positionRate->setValueNotifyingHost(nRate);
        if (positionModA) positionModA->setValueNotifyingHost(nModA);
        if (positionModB) positionModB->setValueNotifyingHost(nModB);

        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.patternState.patternStateLock);
            audioProcessor.patternState.lastPositionPattern = positionPattern;
        }

        audioProcessor.guiState.syncingPosition.store(false, std::memory_order_release);
    }

    if (audioProcessor.guiState.syncingField.load(std::memory_order_acquire)) {
        auto fieldPattern = static_cast<FieldPattern>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load());
        auto* fieldRate = audioProcessor.apvts.getParameter(ParamID::fieldRate);
        auto* fieldModA = audioProcessor.apvts.getParameter(ParamID::fieldModA);
        auto* fieldModB = audioProcessor.apvts.getParameter(ParamID::fieldModB);

        PatternParameterState nMods {};

        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.patternState.patternStateLock);
            audioProcessor.patternState.fieldParamStates[audioProcessor.patternState.lastFieldPattern] =
                { fieldRate->getValue(),
                  fieldModA->getValue(),
                  fieldModB->getValue() };

            nMods = audioProcessor.patternState.fieldParamStates[fieldPattern];
        }

        float nRate = nMods.rate, 
              nModA = nMods.modA, 
              nModB = nMods.modB;

        if (fieldPattern == FieldPattern::MANUAL) {
            PolarCoordinate coordinate;
            {
                juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.fieldLock);
                const int& selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
                coordinate = audioProcessor.guiState.fieldCoordinates[selectedIR];
            }
            nModA = fieldModA->convertTo0to1(coordinate.r);
            nModB = fieldModB->convertTo0to1(coordinate.theta / juce::MathConstants<float>::twoPi);
        }

        if (fieldRate) fieldRate->setValueNotifyingHost(nRate);
        if (fieldModA) fieldModA->setValueNotifyingHost(nModA);
        if (fieldModB) fieldModB->setValueNotifyingHost(nModB);

        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.patternState.patternStateLock);
            audioProcessor.patternState.lastFieldPattern = fieldPattern;
        }

        audioProcessor.guiState.syncingField.store(false, std::memory_order_release);
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
    randomAllButton.onClick = [this]() { audioProcessor.getIRManager()->loadRandomIRs(); };

    clearAllButton.setButtonText("Clear IRs");
    clearAllButton.onClick = [this]() { audioProcessor.getIRManager()->clearIRs(); };

    // Settings modal
    settingsButton.onClick = [this]() {
        if (settingsModal) {
            settingsModal.reset();
        } else {
            settingsModal = std::make_unique<SettingsComponent>(audioProcessor.getIRManager());
            settingsModal->onCloseRequested = [this]() { settingsModal.reset(); };
            addAndMakeVisible(*settingsModal);
            settingsModal->setBounds(getLocalBounds().withSizeKeepingCentre(400, 300));
        }
    };

    // IR slot buttons
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        irSlotButtons[i] = std::make_unique<IRSlotButton>(i);
        irSlotButtons[i]->setRadioGroupId(1);
        irSlotButtons[i]->setClickingTogglesState(true);

        irSlotButtons[i]->onActiveToggle = [this, i](bool active) {
            audioProcessor.getIRManager()->setIRActive(i, active);
            audioProcessor.guiState.updateField.store(true, std::memory_order_release);
        };

        irSlotButtons[i]->onClick = [this, i]() {
            audioProcessor.apvts.state.setProperty(PropertyID::selectedIR, i, nullptr);
            selectedIRChanged.store(true, std::memory_order_relaxed);
            audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        };

        const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
        irSlotButtons[i]->setOccupied(slot.occupied);
        if (slot.occupied) irSlotButtons[i]->setWaveform(&slot.buffer);
        addAndMakeVisible(*irSlotButtons[i]);
    }

    int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    if (validateIRIndex(selectedIR))
        irSlotButtons[selectedIR]->setToggleState(true, juce::dontSendNotification);

    // Selected IR header
    irHeaderComponent.onClear = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) {
            audioProcessor.getIRManager()->clearIR(selectedIR);

            const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
            irHeaderComponent.setSlot(selectedIR, slot);
        }
    };

    irHeaderComponent.onActiveToggle = [this](bool active) {
        audioProcessor.getIRManager()->setIRActive(audioProcessor.apvts.state.getProperty(PropertyID::selectedIR), active);
        audioProcessor.guiState.updateField.store(true, std::memory_order_release);
    };

    // Selected IR waveform
    irWaveformComponent.setDimensions(16.0f, 0.0f, -16.0f, 1.0f);
    irWaveformComponent.setColor(Theme::Colors::main);

    // Selected IR controls
    loadIRButton.setButtonText("Load");
    loadIRButton.onClick = [this]() { 
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR))
            audioProcessor.getIRManager()->chooseIR(selectedIR);
    };

    randomIRButton.setButtonText("Random");
    randomIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR))
            audioProcessor.getIRManager()->loadRandomIR(selectedIR);
    };
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

    // Attach listeners
    for (const auto& id : paramIDs) audioProcessor.apvts.addParameterListener(id, this);

    // Add components
    addAndMakeVisible(polarMapComponent);

    addAndMakeVisible(positionTabButton);
    addAndMakeVisible(fieldTabButton);

    addAndMakeVisible(randomAllButton);
    addAndMakeVisible(clearAllButton);

    addAndMakeVisible(settingsButton);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        irSlotButtons[i] = std::make_unique<IRSlotButton>(i);
        addAndMakeVisible(*irSlotButtons[i]);
    }

    addAndMakeVisible(irHeaderComponent);
    
    addAndMakeVisible(irWaveformComponent);

    addAndMakeVisible(loadIRButton);
    addAndMakeVisible(randomIRButton);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        swapControls[i] = std::make_unique<SwapControl>(audioProcessor.apvts, i);
        addChildComponent(swapControls[i]->swapMinControl);
        addChildComponent(swapControls[i]->swapMaxControl);
    }

    for (auto& control : getControls()) addAndMakeVisible(control.component);

    // Setup components
    initComponents();

    startTimerHz(REFRESH_RATE);
    setSize(1046, 721);
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

    Bounds topBarBounds = rightPanel.removeFromTop(81);
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
    juce::FlexBox topBarRow;

    juce::FlexBox globalIROperations;
    globalIROperations.flexDirection = juce::FlexBox::Direction::column;
    globalIROperations.items.add(juce::FlexItem(randomAllButton).withFlex(1.0f));
    globalIROperations.items.add(juce::FlexItem(clearAllButton).withFlex(1.0f));
    // globalIROperations.performLayout(topBarBounds.removeFromLeft(static_cast<int>(topBarBounds.getWidth() * 0.25f)));

    topBarRow.items.add(juce::FlexItem(globalIROperations).withFlex(1.0f));
    topBarRow.items.add(juce::FlexItem(settingsButton).withFlex(1.0f));

    topBarRow.performLayout(topBarBounds.removeFromLeft(static_cast<int>(topBarBounds.getWidth() * 0.6f)));

    Bounds irGridBounds = rightPanel.removeFromTop(160);
    juce::Grid irGrid;
    irGrid.templateRows =    { juce::Grid::Fr(1), juce::Grid::Fr(1) };
    irGrid.templateColumns = { juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1) };

    for (auto& slot : irSlotButtons) irGrid.items.add(juce::GridItem(*slot));
    irGrid.performLayout(irGridBounds);

    Bounds irHeaderBounds = rightPanel.removeFromTop(40); // IR (indicator color) - filename, Clear button
    irHeaderComponent.setBounds(irHeaderBounds);

    Bounds irWaveformBounds = rightPanel.removeFromTop(140);
    irWaveformComponent.setBounds(irWaveformBounds);

    Bounds irControlsBounds = rightPanel.removeFromTop(100); // Load (Manual) button, Load (Random) button, Auto Min textbox, Auto Max textbox
    juce::FlexBox irControlRow;
    irControlRow.alignItems = juce::FlexBox::AlignItems::center;
    irControlRow.items.add(juce::FlexItem(loadIRButton).withFlex(1.0f).withHeight(irControlsBounds.getHeight() * 0.6f));
    irControlRow.items.add(juce::FlexItem(randomIRButton).withFlex(1.0f).withHeight(irControlsBounds.getHeight() * 0.6f));
    irControlRow.performLayout(irControlsBounds.removeFromLeft(static_cast<int>(irControlsBounds.getWidth() * 0.4f)));

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        juce::FlexBox swapControlsRow;
        swapControlsRow.items.add(juce::FlexItem(swapControls[i]->swapMinControl).withFlex(1.0f));
        swapControlsRow.items.add(juce::FlexItem(swapControls[i]->swapMaxControl).withFlex(1.0f));
        swapControlsRow.performLayout(irControlsBounds);
    }

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