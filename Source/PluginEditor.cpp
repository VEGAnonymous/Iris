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

            irWaveformComponent.setNumPoints(WAVEFORM_POINTS);
            irWaveformComponent.setWaveform(&slot.buffer, audioProcessor.getSampleRate());
            irWaveformComponent.setActive(slot.active);
            
            for (int i = 0; i < MAX_IR_COUNT; ++i) {
                if (irSlotButtons[i]) 
                    irSlotButtons[i]->setToggleState(i == selectedIR, juce::NotificationType::dontSendNotification);

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
            if (irSlotButtons[i]) {
                irSlotButtons[i]->setOccupied(slot.occupied);
                irSlotButtons[i]->setActive(slot.active);
                irSlotButtons[i]->setWaveform(slot.occupied ? &slot.buffer : nullptr, audioProcessor.getSampleRate());
            }
        }

        updateIRSlot();
    }

    if (selectedIRChanged.exchange(false, std::memory_order_acquire))
        updateIRSlot();

    if (polarMapComponent.getIRSwitched().exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        updateIRSlot();
    }

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

    if (audioProcessor.getIRManager()->getDirectoryChanged().exchange(false, std::memory_order_acquire))
        if (settingsModal) settingsModal->refreshDirectories();
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
    randomAllButton.setButtonText("Random All");
    randomAllButton.onClick = [this]() { audioProcessor.getIRManager()->loadRandomIRs(); };

    clearAllButton.setButtonText("Clear All");
    clearAllButton.onClick = [this]() { audioProcessor.getIRManager()->clearIRs(); };

    // Settings modal
    settingsButton.setButtonText("S");
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
        if (slot.occupied) irSlotButtons[i]->setWaveform(&slot.buffer, audioProcessor.getSampleRate());
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
    irWaveformComponent.setDimensions(16.0f, 0.0f, -16.0f, 0.9f);
    irWaveformComponent.setColor(Theme::Colors::highlight);

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

    // Left panel
    Bounds leftPanel = totalBounds.removeFromLeft(621);
    g.setColour(Theme::Colors::background);
    g.fillRect(leftPanel);

    Bounds mapBounds = leftPanel.removeFromTop(621);
    g.setColour(juce::Colours::black);
    g.fillRect(mapBounds);
    g.setColour(juce::Colours::floralwhite.withAlpha(0.4f));
    g.drawRoundedRectangle(mapBounds.toFloat(), 4.0f, 1.0f);

    Bounds& positionFieldControlsBounds = leftPanel;
    g.setColour(Theme::Colors::section);
    g.fillRect(positionFieldControlsBounds.reduced(2));

    // Right panel
    Bounds& rightPanel = totalBounds;
    g.setColour(Theme::Colors::background);
    g.fillRect(rightPanel);

    Bounds topBarBounds = rightPanel.removeFromTop(81);
    g.fillRect(topBarBounds.reduced(2));

    Bounds irGridBounds = rightPanel.removeFromTop(160);
    g.setColour(Theme::Colors::section);
    g.fillRect(irGridBounds.reduced(2));

    Bounds irHeaderBounds = rightPanel.removeFromTop(40);
    g.fillRect(irHeaderBounds.reduced(2));
    
    Bounds irWaveformBounds = rightPanel.removeFromTop(140);
    g.fillRect(irWaveformBounds.reduced(2));

    Bounds irControlsBounds = rightPanel.removeFromTop(100);
    g.fillRect(irControlsBounds.reduced(2));

    Bounds interactionControlsBounds = rightPanel.removeFromTop(100);
    g.fillRect(interactionControlsBounds.reduced(2));

    Bounds globalControlsBounds = rightPanel.removeFromTop(100);
    g.fillRect(globalControlsBounds.reduced(2));
}

void MareverbAudioProcessorEditor::resized() {
    auto fillFlex = [this](juce::FlexBox& flex, ControlGroup group) {
        for (const auto& control : controls)
            if (control.group == group)
                flex.items.add(juce::FlexItem(*control.component).withFlex(1.0f).withHeight(80).withMargin(10));
        };

    Bounds totalBounds = getLocalBounds();

    /* LEFT PANEL */
    Bounds leftPanel = totalBounds.removeFromLeft(621);

    // Mare map
    Bounds mapBounds = leftPanel.removeFromTop(621);
    polarMapComponent.setBounds(mapBounds.reduced(static_cast<int>(leftPanel.getWidth() * 0.05f)));

    // Position + field controls
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

    /* RIGHT PANEL */
    Bounds& rightPanel = totalBounds;

    // Top bar
    Bounds topBarBounds = rightPanel.removeFromTop(81); 
    juce::FlexBox topBarRow;
    topBarRow.alignItems = juce::FlexBox::AlignItems::center;
    const auto topBarWidth = topBarBounds.getWidth(), topBarHeight = topBarBounds.getHeight();
    
    juce::FlexBox globalIROperations;
    globalIROperations.flexDirection = juce::FlexBox::Direction::column;
    globalIROperations.items.add(juce::FlexItem(randomAllButton).withFlex(1.0f));
    globalIROperations.items.add(juce::FlexItem(clearAllButton).withFlex(1.0f));

    topBarRow.items.add(juce::FlexItem(globalIROperations).withFlex(1.0f).withWidth(topBarWidth * 0.25f).withHeight(topBarHeight * 0.8f).withMargin(10));
    topBarRow.items.add(juce::FlexItem(settingsButton).withFlex(1.0f).withWidth(topBarWidth * 0.075f).withHeight(topBarHeight * 0.6f));

    topBarRow.performLayout(topBarBounds.removeFromLeft(static_cast<int>(topBarWidth * 0.4f)));

    // IR selector grid
    Bounds irGridBounds = rightPanel.removeFromTop(160);
    juce::Grid irGrid;
    irGrid.templateRows =    { juce::Grid::Fr(1), juce::Grid::Fr(1) };
    irGrid.templateColumns = { juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1) };

    for (auto& slot : irSlotButtons) irGrid.items.add(juce::GridItem(*slot));
    irGrid.performLayout(irGridBounds);

    // IR header
    Bounds irHeaderBounds = rightPanel.removeFromTop(40);
    irHeaderComponent.setBounds(irHeaderBounds);

    // Selected IR waveform
    Bounds irWaveformBounds = rightPanel.removeFromTop(140);
    irWaveformComponent.setBounds(irWaveformBounds);

    // Selected IR controls
    Bounds irControlsBounds = rightPanel.removeFromTop(100);
    const auto irControlsWidth = irControlsBounds.getWidth(), irControlsHeight = irControlsBounds.getHeight();

    juce::FlexBox irControlRow;
    irControlRow.alignItems = juce::FlexBox::AlignItems::center;
    irControlRow.alignContent = juce::FlexBox::AlignContent::flexStart;
    irControlRow.items.add(juce::FlexItem(loadIRButton).withFlex(1.0f).withHeight(irControlsHeight * 0.7f).withMargin(10));
    irControlRow.items.add(juce::FlexItem(randomIRButton).withFlex(1.0f).withHeight(irControlsHeight * 0.7f).withMargin(10));
    irControlRow.performLayout(irControlsBounds.removeFromLeft(static_cast<int>(irControlsWidth * 0.6f)));

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        juce::FlexBox swapControlsRow;
        swapControlsRow.alignItems = juce::FlexBox::AlignItems::center;
        swapControlsRow.alignContent = juce::FlexBox::AlignContent::flexEnd;
        swapControlsRow.items.add(juce::FlexItem(swapControls[i]->swapMinControl).withFlex(1.0f).withHeight(irControlsHeight * 0.7f).withMargin(5));
        swapControlsRow.items.add(juce::FlexItem(swapControls[i]->swapMaxControl).withFlex(1.0f).withHeight(irControlsHeight * 0.7f).withMargin(5));
        swapControlsRow.performLayout(irControlsBounds);
    }

    // Interaction controls
    Bounds interactionControlsBounds = rightPanel.removeFromTop(100); 
    juce::FlexBox interactionControlRow;
    fillFlex(interactionControlRow, ControlGroup::INTERACTION);
    interactionControlRow.performLayout(interactionControlsBounds);

    // Global controls
    Bounds globalControlsBounds = rightPanel.removeFromTop(100);
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