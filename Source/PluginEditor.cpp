#include "GUIUtilities.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Theme.h"
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
    animatorUpdater.update();

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

    if (audioProcessor.guiState.irChanged.exchange(false, std::memory_order_acquire)) {
        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
            if (irSlotButtons[i]) {
                irSlotButtons[i]->setOccupied(slot.occupied);
                irSlotButtons[i]->setActive(slot.active);
                irSlotButtons[i]->setWaveform(slot.occupied ? &slot.buffer : nullptr, audioProcessor.getSampleRate());
            }
        }

        updateIRSlot(true);
    }

    if (selectedIRChanged.exchange(false, std::memory_order_acquire))
        updateIRSlot(false);

    if (polarMapComponent.getIRSwitched().exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        updateIRSlot(true);
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

void MareverbAudioProcessorEditor::updateIRSlot(bool animate) {
    int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    // DBG("Selected IR " << selectedIR);
    if (validateIRIndex(selectedIR)) {
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
        irHeaderComponent.setActive(slot.active, animate);
        irHeaderComponent.setSlot(selectedIR, slot);

        irWaveformComponent.setNumPoints(WAVEFORM_POINTS);
        irWaveformComponent.setWaveform(&slot.buffer, audioProcessor.getSampleRate());
        irWaveformComponent.setActive(slot.active, animate);

        windowOverlayComponent.setMaxLength(slot.getMaxWindowLength());
        windowOverlayComponent.setWindow(slot.window.start, slot.window.start + slot.window.length);

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

void MareverbAudioProcessorEditor::initComponents() {
    // Weighting mode toggle switch
    weightingModeControl.control.setClickingTogglesState(true);
    auto updateWeightingModeText = [this] { 
        weightingModeControl.control.setButtonText(weightingModes[weightingModeControl.control.getToggleState()]); 
    };
    updateWeightingModeText();
    weightingModeControl.control.onStateChange = updateWeightingModeText;

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
        positionTabButton.setToggleState(true, juce::dontSendNotification);
        fieldTabButton.setToggleState(false, juce::dontSendNotification);
        for (auto& positionControl : positionControls) positionControl.component->setVisible(true);
        for (auto& fieldControl : fieldControls) fieldControl.component->setVisible(false); 
    };

    auto selectFieldTab = [this, positionControls, fieldControls] {
        positionTabButton.setToggleState(false, juce::dontSendNotification);
        fieldTabButton.setToggleState(true, juce::dontSendNotification);
        for (auto& positionControl : positionControls) positionControl.component->setVisible(false);
        for (auto& fieldControl : fieldControls) fieldControl.component->setVisible(true);
    };

    positionTabButton.setButtonText("POSITION");
    positionTabButton.setToggleable(true);
    positionTabButton.onClick = selectPositionTab;

    fieldTabButton.setButtonText("FIELD");
    fieldTabButton.setToggleable(true);
    fieldTabButton.onClick = selectFieldTab;

    selectPositionTab();

    // Randomize / clear all buttons
    randomAllButton.setButtonText("RANDOM ALL");
    randomAllButton.onClick = [this]() { audioProcessor.getIRManager()->loadRandomIRs(); };

    clearAllButton.setButtonText("CLEAR ALL");
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
        irSlotButtons[i] = std::make_unique<IRSlotButton>(i, animatorUpdater);
        irSlotButtons[i]->setRadioGroupId(1);
        irSlotButtons[i]->setClickingTogglesState(true);

        irSlotButtons[i]->onActiveToggle = [this, i](bool active) {
            audioProcessor.getIRManager()->setIRActive(i, active);
            audioProcessor.guiState.updateField.store(true, std::memory_order_release);
        };

        irSlotButtons[i]->onClick = [this, i]() {
            int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
            if (i != selectedIR) {
                audioProcessor.apvts.state.setProperty(PropertyID::selectedIR, i, nullptr);
                selectedIRChanged.store(true, std::memory_order_relaxed);
                audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
            }
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
    irHeaderComponent.onActiveToggle = [this](bool active) {
        audioProcessor.getIRManager()->setIRActive(audioProcessor.apvts.state.getProperty(PropertyID::selectedIR), active);
        audioProcessor.guiState.updateField.store(true, std::memory_order_release);
    };

    // Selected IR waveform
    irWaveformComponent.setDimensions(16.0f, 0.0f, -16.0f, 0.9f);
    irWaveformComponent.setColor(Theme::Colors::highlight);

    windowOverlayComponent.onWindowChanged = [this](float start, float length) {
        audioProcessor.getIRManager()->setWindow(audioProcessor.apvts.state.getProperty(PropertyID::selectedIR), start, length);
    };

    // Selected IR controls
    loadIRButton.setButtonText("LOAD");
    loadIRButton.onClick = [this]() { 
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR))
            audioProcessor.getIRManager()->chooseIR(selectedIR);
    };

    clearIRButton.setButtonText("CLEAR");
    clearIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR))
            audioProcessor.getIRManager()->clearIR(selectedIR);
    };

    randomIRButton.setButtonText("RANDOM");
    randomIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR))
            audioProcessor.getIRManager()->loadRandomIR(selectedIR);
    };

    // Misc
    weightingModeControl.setControlHeight(40);
}

void MareverbAudioProcessorEditor::fillFlex(juce::FlexBox& flexBox, ControlGroup group, 
    juce::FlexItem::Margin margin, float flex, float width, float height) {
    for (const auto& control : controls)
        if (control.group == group)
            flexBox.items.add(juce::FlexItem(*control.component)
                .withFlex(flex)
                .withWidth(width)
                .withHeight(height)
                .withMargin(margin));
}

void MareverbAudioProcessorEditor::layoutLeftPanel(Bounds bounds) {
    Bounds mapBounds = bounds.removeFromTop(621);
    polarMapComponent.setBounds(mapBounds.reduced(static_cast<int>(mapBounds.getWidth() * 0.05f))); // Mare map

    layoutPositionFieldControls(bounds);
}

void MareverbAudioProcessorEditor::layoutRightPanel(Bounds bounds) {
    layoutTopBar(bounds.removeFromTop(81));
    layoutIRSelectorGrid(bounds.removeFromTop(160));
    layoutSelectedIR(bounds.removeFromTop(280));
    layoutInteractionControls(bounds.removeFromTop(100));
    layoutGlobalControls(bounds.removeFromTop(100));
}

void MareverbAudioProcessorEditor::layoutPositionFieldControls(Bounds bounds) {
    // Position / field tab selector column
    juce::FlexBox positionFieldTabs(juce::FlexBox::JustifyContent::center);
    positionFieldTabs.flexDirection = juce::FlexBox::Direction::column;

    positionFieldTabs.items.add(juce::FlexItem(positionTabButton)
        .withFlex(1.0f)
        .withMargin(juce::FlexItem::Margin(10.0f, 8.0f, 5.0f, 10.0f)));

    positionFieldTabs.items.add(juce::FlexItem(fieldTabButton)
        .withFlex(1.0f)
        .withMargin(juce::FlexItem::Margin(5.0f, 8.0f, 10.0f, 10.0f)));

    positionFieldTabs.performLayout(bounds.removeFromLeft(static_cast<int>(bounds.getWidth() * 0.2f)));

    // Position control row
    const float comboBoxWidth = 100.0f, comboBoxHeight = 30.0f;
    const float rotaryWidth = 70.0f, rotaryHeight = 80.0f;
    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f);

    juce::FlexBox positionControlRow(juce::FlexBox::JustifyContent::flexEnd);
    positionControlRow.alignItems = juce::FlexBox::AlignItems::center;

    positionControlRow.items.add(juce::FlexItem(positionPatternControl)
        .withFlex(1.0f)
        .withWidth(comboBoxWidth)
        .withHeight(comboBoxHeight)
        .withMargin(rowItemMargin));

    std::vector<LabelledControl<Rotary>*> positionRotaries { &positionRateControl, &positionModAControl, &positionModBControl };
    for (auto* rotary : positionRotaries)
        positionControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));

    // Field control row
    juce::FlexBox fieldControlRow(juce::FlexBox::JustifyContent::flexEnd);
    fieldControlRow.alignItems = juce::FlexBox::AlignItems::center;

    fieldControlRow.items.add(juce::FlexItem(fieldPatternControl)
        .withFlex(1.0f)
        .withWidth(comboBoxWidth)
        .withHeight(comboBoxHeight)
        .withMargin(rowItemMargin));
    
    std::vector<LabelledControl<Rotary>*> fieldRotaries { &fieldRateControl, &fieldModAControl, &fieldModBControl };
    for (auto* rotary : fieldRotaries)
        fieldControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));

    // Layout
    positionControlRow.performLayout(bounds);
    fieldControlRow.performLayout(bounds);
}

void MareverbAudioProcessorEditor::layoutTopBar(Bounds bounds) {
    const auto w = bounds.getWidth(), 
               h = bounds.getHeight();

    juce::FlexBox topBarRow(juce::FlexBox::JustifyContent::flexStart);
    topBarRow.alignItems = juce::FlexBox::AlignItems::center;

    juce::FlexBox globalIROperations(juce::FlexBox::JustifyContent::center);
    globalIROperations.flexDirection = juce::FlexBox::Direction::column;

    globalIROperations.items.add(juce::FlexItem(randomAllButton).withFlex(1.0f)); // 'Random All' button
    globalIROperations.items.add(juce::FlexItem(clearAllButton).withFlex(1.0f));  // 'Clear All' button

    // Layout
    topBarRow.items.add(juce::FlexItem(globalIROperations)
        .withFlex(0.0f)
        .withWidth(w * 0.15f)
        .withHeight(h * 0.8f)
        .withMargin(22.5f));

    topBarRow.items.add(juce::FlexItem(settingsButton) // Settings modal button
        .withFlex(0.0f)
        .withWidth(w * 0.075f)
        .withHeight(h * 0.6f));

    topBarRow.performLayout(bounds.removeFromLeft(static_cast<int>(w * 0.4f)));
}

void MareverbAudioProcessorEditor::layoutIRSelectorGrid(Bounds bounds) {
    juce::Grid irGrid; // 2x4
    irGrid.templateRows =    { juce::Grid::Fr(1), juce::Grid::Fr(1) };
    irGrid.templateColumns = { juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1), juce::Grid::Fr(1) };

    for (auto& slot : irSlotButtons) irGrid.items.add(juce::GridItem(*slot));
    irGrid.performLayout(bounds);
}

void MareverbAudioProcessorEditor::layoutSelectedIR(Bounds bounds) {
    irHeaderComponent.setBounds(bounds.removeFromTop(40));

    irWaveformComponent.setBounds(bounds.removeFromTop(140));
    windowOverlayComponent.setBounds(irWaveformComponent.getBounds().withTrimmedLeft(16).withTrimmedRight(16));

    layoutIRControls(bounds.removeFromTop(100));
}

void MareverbAudioProcessorEditor::layoutIRControls(Bounds bounds) {
    const auto w = bounds.getWidth(), 
               h = bounds.getHeight();

    juce::FlexBox irControlRow(juce::FlexBox::JustifyContent::flexStart);
    irControlRow.alignItems = juce::FlexBox::AlignItems::center;

    // 'Load' / 'Clear' button
    juce::FlexBox irControlColumn(juce::FlexBox::JustifyContent::center);
    irControlColumn.flexDirection = juce::FlexBox::Direction::column;

    irControlColumn.items.add(juce::FlexItem(loadIRButton)
        .withFlex(1.0f)
        .withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 3.0f, 0.0f)));

    irControlColumn.items.add(juce::FlexItem(clearIRButton)
        .withFlex(1.0f)
        .withMargin(juce::FlexItem::Margin(3.0f, 0.0f, 0.0f, 0.0f)));

    // Layout
    irControlRow.items.add(juce::FlexItem(irControlColumn)
        .withFlex(0.0f)
        .withWidth(w * 0.15f)
        .withHeight(h * 0.75f)
        .withMargin(22.5f));

    irControlRow.items.add(juce::FlexItem(randomIRButton) // 'Random' button
        .withFlex(0.0f)
        .withWidth(w * 0.25f)
        .withHeight(h * 0.5f)
        .withMargin(juce::FlexItem::Margin(10.0f, 15.0f, 10.0f, 10.0f)));

    irControlRow.performLayout(bounds.removeFromLeft(static_cast<int>(w * 0.6f)));

    // Swap controls
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        juce::FlexBox swapControlsRow(juce::FlexBox::JustifyContent::center);
        swapControlsRow.alignItems = juce::FlexBox::AlignItems::center;

        swapControlsRow.items.add(juce::FlexItem(swapControls[i]->swapMinControl) // 'Auto Min' knob
            .withFlex(1.0f)
            .withWidth(60.0f)
            .withHeight(70.0f)
            .withMargin(10));

        swapControlsRow.items.add(juce::FlexItem(swapControls[i]->swapMaxControl) // 'Auto Max' knob
            .withFlex(1.0f)
            .withWidth(60.0f)
            .withHeight(70.0f)
            .withMargin(10));

        swapControlsRow.performLayout(bounds);
    }
}

void MareverbAudioProcessorEditor::layoutInteractionControls(Bounds bounds) {
    juce::FlexBox interactionControlRow(juce::FlexBox::JustifyContent::center);
    interactionControlRow.alignItems = juce::FlexBox::AlignItems::center;

    fillFlex(interactionControlRow, ControlGroup::INTERACTION, 
        juce::FlexItem::Margin(10.0f, 30.0f, 10.0f, 30.0f), 0.0f, 68.0f, 80.0f);

    interactionControlRow.performLayout(bounds);
}

void MareverbAudioProcessorEditor::layoutGlobalControls(Bounds bounds) {
    juce::FlexBox globalControlRow(juce::FlexBox::JustifyContent::center);
    globalControlRow.alignItems = juce::FlexBox::AlignItems::center;

    fillFlex(globalControlRow, ControlGroup::GLOBAL,
        juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f), 0.0f, 68.0f, 80.0f);

    globalControlRow.performLayout(bounds);
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor (MareverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),

    polarMapComponent(audioProcessor),

    globalMixControlAttachment(audioProcessor.apvts, ParamID::globalMix, globalMixControl.control),
    decayControlAttachment(audioProcessor.apvts, ParamID::decay, decayControl.control),
    lowCutControlAttachment(audioProcessor.apvts, ParamID::lowCut, lowCutControl.control),
    highCutControlAttachment(audioProcessor.apvts, ParamID::highCut, highCutControl.control),

    weightingModeControlAttachment(audioProcessor.apvts, ParamID::weightingMode, weightingModeControl.control),
    strengthControlAttachment(audioProcessor.apvts, ParamID::strength, strengthControl.control),
    spreadControlAttachment(audioProcessor.apvts, ParamID::spread, spreadControl.control),

    positionPatternControlAttachment(audioProcessor.apvts, ParamID::positionPattern, positionPatternControl),
    positionRateControlAttachment(audioProcessor.apvts, ParamID::positionRate, positionRateControl.control),
    positionModAControlAttachment(audioProcessor.apvts, ParamID::positionModA, positionModAControl.control),
    positionModBControlAttachment(audioProcessor.apvts, ParamID::positionModB, positionModBControl.control),

    fieldPatternControlAttachment(audioProcessor.apvts, ParamID::fieldPattern, fieldPatternControl),
    fieldRateControlAttachment(audioProcessor.apvts, ParamID::fieldRate, fieldRateControl.control),
    fieldModAControlAttachment(audioProcessor.apvts, ParamID::fieldModA, fieldModAControl.control),
    fieldModBControlAttachment(audioProcessor.apvts, ParamID::fieldModB, fieldModBControl.control),

    irHeaderComponent(animatorUpdater), irWaveformComponent(animatorUpdater), windowOverlayComponent(animatorUpdater) {

    // Attach listeners
    for (const auto& id : paramIDs) audioProcessor.apvts.addParameterListener(id, this);

    // Add components
    addAndMakeVisible(polarMapComponent);

    positionTabButton.setLookAndFeel(&buttonLookAndFeel);
    fieldTabButton.setLookAndFeel(&buttonLookAndFeel);
    addAndMakeVisible(positionTabButton);
    addAndMakeVisible(fieldTabButton);

    randomAllButton.setLookAndFeel(&buttonLookAndFeel);
    clearAllButton.setLookAndFeel(&buttonLookAndFeel);
    addAndMakeVisible(randomAllButton);
    addAndMakeVisible(clearAllButton);

    settingsButton.setLookAndFeel(&buttonLookAndFeel);
    addAndMakeVisible(settingsButton);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        irSlotButtons[i] = std::make_unique<IRSlotButton>(i, animatorUpdater);
        addAndMakeVisible(*irSlotButtons[i]);
    }

    addAndMakeVisible(irHeaderComponent);
    
    addAndMakeVisible(irWaveformComponent);
    addAndMakeVisible(windowOverlayComponent);

    loadIRButton.setLookAndFeel(&buttonLookAndFeel);
    clearIRButton.setLookAndFeel(&buttonLookAndFeel);
    randomIRButton.setLookAndFeel(&buttonLookAndFeel);
    addAndMakeVisible(loadIRButton);
    addAndMakeVisible(clearIRButton);
    addAndMakeVisible(randomIRButton);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        swapControls[i] = std::make_unique<SwapControl>(audioProcessor.apvts, animatorUpdater, i);
        swapControls[i]->swapMinControl.setLookAndFeel(&rotaryLookAndFeel);
        swapControls[i]->swapMaxControl.setLookAndFeel(&rotaryLookAndFeel);

        addChildComponent(swapControls[i]->swapMinControl);
        addChildComponent(swapControls[i]->swapMaxControl);
    }

    // Setup base controls 
    for (auto& control : getControls()) {
        control.applyLookAndFeel();
        addAndMakeVisible(control.component);
    }

    // Setup components
    initComponents();

    startTimerHz(REFRESH_RATE);
    setSize(1046, 721);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() {
    // Detach look and feel
    for (auto& control : getControls()) control.component->setLookAndFeel(nullptr);

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
    g.setColour(Theme::Colors::background);
    g.fillRect(irWaveformBounds.reduced(2));
    g.setColour(juce::Colours::floralwhite.withAlpha(0.4f));
    g.drawRoundedRectangle(irWaveformBounds.reduced(1).toFloat(), 4.0f, 0.8f);

    Bounds irControlsBounds = rightPanel.removeFromTop(100);
    g.setColour(Theme::Colors::section);
    g.fillRect(irControlsBounds.reduced(2));

    Bounds interactionControlsBounds = rightPanel.removeFromTop(100);
    g.fillRect(interactionControlsBounds.reduced(2));

    Bounds globalControlsBounds = rightPanel.removeFromTop(100);
    g.fillRect(globalControlsBounds.reduced(2));
}

void MareverbAudioProcessorEditor::resized() {
    Bounds bounds = getLocalBounds();
    layoutLeftPanel(bounds.removeFromLeft(621));
    layoutRightPanel(bounds);
}

std::vector<ControlDef> MareverbAudioProcessorEditor::getControls() { return controls; }

std::vector<ControlDef> MareverbAudioProcessorEditor::getControlsGroup(ControlGroup group) {
    std::vector<ControlDef> groupControls;
    for (const auto& control : controls)
        if (control.group == group)
            groupControls.push_back(control);
    return groupControls;
}