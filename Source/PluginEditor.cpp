#include "GUIUtilities.h"
#include "LabelledControl.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Theme.h"
#include "Utilities.h"

/* PRIVATE */

// Listeners and callbacks

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    // DBG("Parameter changed");
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

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        if (parameterID == ParamID::irSwapActive(i) /*|| parameterID == ParamID::irSwapMin(i) || parameterID == ParamID::irSwapMax(i)*/) {
            audioProcessor.guiState.swapChanged.store(true, std::memory_order_release);
        }
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

    if (audioProcessor.guiState.swapChanged.exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingSwap.store(true, std::memory_order_release);
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        bool swapActive = swapControls[selectedIR]->swapActiveToggle.control.getToggleState();

        swapControls[selectedIR]->swapRangeSlider.setEnabled(swapActive);
        swapControls[selectedIR]->swapRangeSlider.repaint();
        audioProcessor.getIRManager()->setSwapActive(selectedIR, swapActive);
        audioProcessor.guiState.syncingSwap.store(false, std::memory_order_release);
        // DBG("Swap changed");
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

    if (audioProcessor.guiState.syncingPosition.load(std::memory_order_acquire))
        syncPosition();

    if (audioProcessor.guiState.syncingField.load(std::memory_order_acquire))
        syncField();

    if (audioProcessor.getIRManager()->getDirectoryChanged().exchange(false, std::memory_order_acquire))
        if (settingsModal) settingsModal->refreshDirectories();
}

// GUI state

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

        envelopeComponent.setSlot(slot);

        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            if (irSlotButtons[i]) irSlotButtons[i]->setToggleState(i == selectedIR, juce::NotificationType::dontSendNotification);
            if (swapControls[i]) {
                swapControls[i]->swapActiveToggle.setVisible(i == selectedIR);
                swapControls[i]->swapRangeSlider.setVisible(i == selectedIR);
            }
        }

        const float swapMin = juce::jmap(slot.autoSwap.minTime, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX, 0.0f, 1.0f); // normalized
        const float swapMax = juce::jmap(slot.autoSwap.maxTime, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX, 0.0f, 1.0f);
        swapControls[selectedIR]->swapRangeSlider.control.setRange(swapMin, swapMax);
    }
};

void MareverbAudioProcessorEditor::syncPosition() {
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

    positionRateControl.setEnabled(true);
    positionModAControl.setEnabled(true);
    positionModBControl.setEnabled(true);

    // Set label text, enabled/disabled for each pattern
    switch (positionPattern) {
        case PositionPattern::MANUAL: {
            positionModAControl.label.setText("Radius", juce::dontSendNotification);
            positionModBControl.label.setText("Angle", juce::dontSendNotification);
            positionRateControl.setEnabled(false);

            PolarCoordinate position = audioProcessor.guiState.position.load();
            nModA = positionModA->convertTo0to1(position.r);
            nModB = positionModB->convertTo0to1(position.theta / juce::MathConstants<float>::twoPi);
            break;
        }
        case PositionPattern::EYES: {
            positionModAControl.label.setText("Angle", juce::dontSendNotification);
            positionModBControl.label.setText("-", juce::dontSendNotification);
            positionModBControl.setEnabled(false);
            break;
        }
        case PositionPattern::ORBIT: {
            positionModAControl.label.setText("Radius", juce::dontSendNotification);
            positionModBControl.label.setText("-", juce::dontSendNotification);
            positionModBControl.setEnabled(false);
            break;
        }
        case PositionPattern::SPIRAL: {
            positionModAControl.label.setText("Swirl", juce::dontSendNotification);
            positionModBControl.label.setText("-", juce::dontSendNotification);
            positionModBControl.setEnabled(false);
            break;
        }
        case PositionPattern::FLORAL:
        case PositionPattern::LISSAJOUS: {
            positionModAControl.label.setText("P", juce::dontSendNotification);
            positionModBControl.label.setText("Q", juce::dontSendNotification);
            break;
        }
        case PositionPattern::RANDOM_DISCRETE: {
            positionModAControl.label.setText("Radius", juce::dontSendNotification);
            positionModBControl.label.setText("Smoothing", juce::dontSendNotification);
            break;
        }
        case PositionPattern::RANDOM_WALK: {
            positionModAControl.label.setText("Wander", juce::dontSendNotification);
            positionModBControl.label.setText("Bounce", juce::dontSendNotification);
            break;
        }
        default: {
            positionModAControl.label.setText(positionModAControl.paramName, juce::dontSendNotification);
            positionModBControl.label.setText(positionModBControl.paramName, juce::dontSendNotification);
        }
    }

    positionRateControl.repaint();
    positionModAControl.repaint();
    positionModBControl.repaint();

    if (positionRate) positionRate->setValueNotifyingHost(nRate);
    if (positionModA) positionModA->setValueNotifyingHost(nModA);
    if (positionModB) positionModB->setValueNotifyingHost(nModB);

    {
        juce::SpinLock::ScopedLockType lock(audioProcessor.patternState.patternStateLock);
        audioProcessor.patternState.lastPositionPattern = positionPattern;
    }

    audioProcessor.guiState.syncingPosition.store(false, std::memory_order_release);
}

void MareverbAudioProcessorEditor::syncField() {
    auto fieldPattern = static_cast<FieldPattern>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load());
    auto* fieldRate = audioProcessor.apvts.getParameter(ParamID::fieldRate);
    auto* fieldModA = audioProcessor.apvts.getParameter(ParamID::fieldModA);
    auto* fieldModB = audioProcessor.apvts.getParameter(ParamID::fieldModB);

    PatternParameterState nMods{};

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

    fieldRateControl.setEnabled(true);
    fieldModAControl.setEnabled(true);
    fieldModBControl.setEnabled(true);

    // Set label text, enabled/disabled for each pattern
    switch (fieldPattern) {
        case FieldPattern::MANUAL: {
            fieldRateControl.setEnabled(false);
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Angle", juce::dontSendNotification);

            PolarCoordinate coordinate;
            {
                juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.fieldLock);
                const int& selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
                coordinate = audioProcessor.guiState.fieldCoordinates[selectedIR];
            }
            nModA = fieldModA->convertTo0to1(coordinate.r);
            nModB = fieldModB->convertTo0to1(coordinate.theta / juce::MathConstants<float>::twoPi);
            break;
        }
        case FieldPattern::RING: {
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Offset", juce::dontSendNotification);
            break;
        }
        case FieldPattern::ORBITS: {
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Bias", juce::dontSendNotification);
            break;
        }
        case FieldPattern::RANDOM_DISCRETE: {
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Smoothing", juce::dontSendNotification);
            break;
        }
        case FieldPattern::RANDOM_WALK: {
            fieldModAControl.label.setText("Wander", juce::dontSendNotification);
            fieldModBControl.label.setText("Bounce", juce::dontSendNotification);
            break;
        }
    
    }
    
    fieldRateControl.repaint();
    fieldModAControl.repaint();
    fieldModBControl.repaint();

    if (fieldRate) fieldRate->setValueNotifyingHost(nRate);
    if (fieldModA) fieldModA->setValueNotifyingHost(nModA);
    if (fieldModB) fieldModB->setValueNotifyingHost(nModB);

    {
        juce::SpinLock::ScopedLockType lock(audioProcessor.patternState.patternStateLock);
        audioProcessor.patternState.lastFieldPattern = fieldPattern;
    }

    audioProcessor.guiState.syncingField.store(false, std::memory_order_release);
}

// Components

void MareverbAudioProcessorEditor::initPositionFieldControls() {
    auto positionControls = getControlsGroup(ControlGroup::POSITION);
    auto fieldControls = getControlsGroup(ControlGroup::FIELD);

    auto selectPositionTab = [this, positionControls, fieldControls] {
        positionTabButton.setToggleState(true, juce::dontSendNotification);
        fieldTabButton.setToggleState(false, juce::dontSendNotification);
        for (auto& positionControl : positionControls) positionControl.component->setVisible(true);
        for (auto& fieldControl : fieldControls) fieldControl.component->setVisible(false);
        audioProcessor.guiState.syncingPosition.store(true);
    };

    auto selectFieldTab = [this, positionControls, fieldControls] {
        positionTabButton.setToggleState(false, juce::dontSendNotification);
        fieldTabButton.setToggleState(true, juce::dontSendNotification);
        for (auto& positionControl : positionControls) positionControl.component->setVisible(false);
        for (auto& fieldControl : fieldControls) fieldControl.component->setVisible(true);
        audioProcessor.guiState.syncingField.store(true);
    };

    positionTabButton.setButtonText("POSITION");
    positionTabButton.setToggleable(true);
    positionTabButton.onClick = selectPositionTab;

    fieldTabButton.setButtonText("FIELD");
    fieldTabButton.setToggleable(true);
    fieldTabButton.onClick = selectFieldTab;

    selectPositionTab();

    // Pattern combo boxes
    positionPatternControl.addItemList(positionPatterns, 1);
    positionPatternControl.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::positionPattern)->load()) + 1, juce::dontSendNotification);

    fieldPatternControl.addItemList(fieldPatterns, 1);
    fieldPatternControl.setSelectedId(
        static_cast<int>(audioProcessor.apvts.getRawParameterValue(ParamID::fieldPattern)->load()) + 1, juce::dontSendNotification);
}

void MareverbAudioProcessorEditor::initTopBar() {
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
            settingsModal = std::make_unique<SettingsComponent>(audioProcessor.getIRManager(), animatorUpdater);
            settingsModal->onCloseRequested = [this]() { juce::MessageManager::callAsync([this]() { settingsModal.reset(); }); };
            settingsModal->setLookAndFeel(&buttonLookAndFeel);
            settingsModal->setBounds(getLocalBounds().withSizeKeepingCentre(462, 361));
            addAndMakeVisible(*settingsModal);
        }
    };
}

void MareverbAudioProcessorEditor::initIRSlotButtons() {
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
}

void MareverbAudioProcessorEditor::initSelectedIR() {
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

    envelopeComponent.onEnvelopeChanged = [this](EnvelopeType type, float atk, float rel) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR))
            audioProcessor.getIRManager()->setEnvelope(selectedIR, type, atk, rel);
        };

    // Swap controls
    // TODO: Replace this with ParameterAttachment for RangeSlider
    for (int i = 0; i < swapControls.size(); ++i) {
        swapControls[i]->swapRangeSlider.control.onRangeChanged = [this, i](float swapMin, float swapMax /* normalized */) {
            const float minTime = juce::jmap(swapMin, 0.0f, 1.0f, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX); // seconds
            const float maxTime = juce::jmap(swapMax, 0.0f, 1.0f, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX);

            // Set parameters
            audioProcessor.apvts.getParameter(ParamID::irSwapMin(i))->setValueNotifyingHost(
                audioProcessor.apvts.getParameter(ParamID::irSwapMin(i))->convertTo0to1(minTime));
            audioProcessor.apvts.getParameter(ParamID::irSwapMax(i))->setValueNotifyingHost(
                audioProcessor.apvts.getParameter(ParamID::irSwapMax(i))->convertTo0to1(maxTime));
            };
    }
}

void MareverbAudioProcessorEditor::initComponents() {
    // Weighting mode toggle switch
    // TODO: Change from TextButton to ToggleSwitch-esque component
    weightingModeControl.control.setClickingTogglesState(true);
    auto updateWeightingModeText = [this] {
        weightingModeControl.control.setButtonText(weightingModes[weightingModeControl.control.getToggleState()]);
    };
    updateWeightingModeText();
    weightingModeControl.control.onStateChange = updateWeightingModeText;

    initPositionFieldControls();
    initTopBar();
    initIRSlotButtons();
    initSelectedIR();
}

// Layout

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
    const float labelHeight = 12.0f;
    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f);

    juce::FlexBox positionControlRow(juce::FlexBox::JustifyContent::flexEnd);
    positionControlRow.alignItems = juce::FlexBox::AlignItems::center;

    positionControlRow.items.add(juce::FlexItem(positionPatternControl)
        .withFlex(1.0f)
        .withWidth(comboBoxWidth)
        .withHeight(comboBoxHeight)
        .withMargin(rowItemMargin));

    std::vector<LabelledControl<Rotary>*> positionRotaries { &positionRateControl, &positionModAControl, &positionModBControl };
    for (auto* rotary : positionRotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        positionControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

    // Field control row
    juce::FlexBox fieldControlRow(juce::FlexBox::JustifyContent::flexEnd);
    fieldControlRow.alignItems = juce::FlexBox::AlignItems::center;

    fieldControlRow.items.add(juce::FlexItem(fieldPatternControl)
        .withFlex(1.0f)
        .withWidth(comboBoxWidth)
        .withHeight(comboBoxHeight)
        .withMargin(rowItemMargin));
    
    std::vector<LabelledControl<Rotary>*> fieldRotaries { &fieldRateControl, &fieldModAControl, &fieldModBControl };
    for (auto* rotary : fieldRotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        fieldControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

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

    // 'Random' / 'Clear' button
    juce::FlexBox irControlColumn(juce::FlexBox::JustifyContent::center);
    irControlColumn.flexDirection = juce::FlexBox::Direction::column;

    irControlColumn.items.add(juce::FlexItem(randomIRButton)
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

    irControlRow.items.add(juce::FlexItem(envelopeComponent) // Window envelope
        .withFlex(1.0f)
        .withWidth(w * 0.25f)
        .withHeight(h * 0.8f)
        .withMargin(10.0f));

    irControlRow.performLayout(bounds.removeFromLeft(static_cast<int>(w * 0.6f)));

    // Swap controls
    Bounds swapToggleBounds = bounds.removeFromTop(30);
    Bounds swapSliderBounds = bounds;
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        auto& activeToggle = swapControls[i]->swapActiveToggle;
        auto& rangeSlider = swapControls[i]->swapRangeSlider;

        activeToggle.setBounds(swapToggleBounds.reduced(4));
        activeToggle.setLabelPosition(LabelledControl<HoverableToggleButton>::LabelPosition::LEFT);
        activeToggle.setLabelDimensions(66.0f, 14.0f);
        activeToggle.setControlDimensions(12.0f, 12.0f);
        activeToggle.setLabelMargin(juce::FlexItem::Margin(15.0f, 0.0f, 5.0f, 0.0f));
        activeToggle.setControlMargin(juce::FlexItem::Margin(15.0f, 5.0f, 5.0f, 8.0f));
        activeToggle.resized();

        rangeSlider.setBounds(swapSliderBounds.reduced(4));
        rangeSlider.setLabelDimensions(92.0f, 14.0f);
        rangeSlider.setControlDimensions(130.0f, 30.0f);
        rangeSlider.setLabelMargin(juce::FlexItem::Margin(0.0f, 0.0f, 2.0f, 0.0f));
        rangeSlider.setControlMargin(juce::FlexItem::Margin(5.0f, 0.0f, 5.0f, 0.0f));
        rangeSlider.resized();
    }
}

void MareverbAudioProcessorEditor::layoutInteractionControls(Bounds bounds) {
    juce::FlexBox interactionControlRow(juce::FlexBox::JustifyContent::center);
    interactionControlRow.alignItems = juce::FlexBox::AlignItems::center;

    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 30.0f, 10.0f, 30.0f);
    const float labelHeight = 12.0f;

    weightingModeControl.setLabelDimensions(68.0f, 12.0f);
    weightingModeControl.setControlDimensions(70.0f, 40.0f);
    weightingModeControl.setControlMargin(juce::FlexItem::Margin(0.0f, 0.0f, 12.5f, 0.0f));
    weightingModeControl.flex.justifyContent = juce::FlexBox::JustifyContent::flexEnd;
    interactionControlRow.items.add(juce::FlexItem(weightingModeControl)
        .withFlex(0.0f)
        .withWidth(68.0f)
        .withHeight(80.0f)
        .withMargin(rowItemMargin));

    const float rotaryWidth = 70.0f, rotaryHeight = 80.0f;
    std::vector<LabelledControl<Rotary>*> interactionRotaries{ &strengthControl, &spreadControl };
    for (auto* rotary : interactionRotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        interactionControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

    interactionControlRow.performLayout(bounds);
}

void MareverbAudioProcessorEditor::layoutGlobalControls(Bounds bounds) {
    juce::FlexBox globalControlRow(juce::FlexBox::JustifyContent::center);
    globalControlRow.alignItems = juce::FlexBox::AlignItems::center;

    const auto rowItemMargin = juce::FlexItem::Margin(10.0f, 20.0f, 10.0f, 20.0f);
    const float labelHeight = 12.0f;

    const float rotaryWidth = 70.0f, rotaryHeight = 80.0f;
    std::vector<LabelledControl<Rotary>*> globalRotaries { &lowCutControl, &highCutControl, &decayControl, &globalMixControl };
    for (auto* rotary : globalRotaries) {
        rotary->setLabelDimensions(rotaryWidth - 6.0f, labelHeight);
        rotary->setControlDimensions(rotaryWidth, rotaryHeight - labelHeight);
        globalControlRow.items.add(juce::FlexItem(*rotary)
            .withFlex(0.0f)
            .withWidth(rotaryWidth)
            .withHeight(rotaryHeight)
            .withMargin(rowItemMargin));
    }

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

    // Selected IR controls
    loadIRButton.setLookAndFeel(&buttonLookAndFeel);
    clearIRButton.setLookAndFeel(&buttonLookAndFeel);
    randomIRButton.setLookAndFeel(&buttonLookAndFeel);
    addAndMakeVisible(loadIRButton);
    addAndMakeVisible(clearIRButton);
    addAndMakeVisible(randomIRButton);

    addAndMakeVisible(envelopeComponent);

    // Swap controls
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        swapControls[i] = std::make_unique<SwapControl>(audioProcessor.apvts, animatorUpdater, i);
        swapControls[i]->swapActiveToggle.control.setLookAndFeel(&buttonLookAndFeel);
        addChildComponent(swapControls[i]->swapActiveToggle);
        addChildComponent(swapControls[i]->swapRangeSlider);
    }

    /* Setup base controls */
    for (auto& control : controls) {
        control.applyLookAndFeel();

        // Bind control formatting to apvts parameter formatting
        if (control.slider && control.paramID.isNotEmpty()) {
            if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(control.paramID))) {
                control.slider->textFromValueFunction = [param](double value) {
                    return param->getText(param->convertTo0to1(static_cast<float>(value)), 0); 
                };
                control.slider->valueFromTextFunction = [param](const juce::String& text) {
                    return static_cast<double>(param->convertFrom0to1(param->getValueForText(text)));
                };
            }
        }

        audioProcessor.apvts.addParameterListener(control.paramID, this);
        addAndMakeVisible(*control.component);
    }

    for (int i = 0; i < MAX_IR_COUNT; ++i) audioProcessor.apvts.addParameterListener(ParamID::irSwapActive(i), this);

    // Setup components
    initComponents();

    startTimerHz(REFRESH_RATE);
    setSize(1046, 721);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() {
    // Detach look and feel
    for (auto& control : getControls()) control.component->setLookAndFeel(nullptr);

    // Detach listeners
    for (auto& control : controls) audioProcessor.apvts.removeParameterListener(control.paramID, this);
    for (int i = 0; i < MAX_IR_COUNT; ++i) audioProcessor.apvts.removeParameterListener(ParamID::irSwapActive(i), this);
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