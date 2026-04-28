#include "Core/Utilities.h"
#include "Core/Control/MotionController.h"
#include "GUI/GUIUtilities.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Theme/Theme.h"
#include "GUI/API/ValueTooltipClient.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

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
            auto* slotButton = irSelectorPanel.getIRSlotButton(i);
            slotButton->setOccupied(slot.occupied);
            slotButton->setActive(slot.active);
            slotButton->setWaveform(slot.occupied ? &slot.buffer : nullptr, audioProcessor.getSampleRate());
        }

        updateIRSlot(true);
    }

    if (audioProcessor.guiState.selectedIRChanged.exchange(false, std::memory_order_acquire))
        updateIRSlot(false);

    if (polarMapComponent.getIRSwitched().exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        updateIRSlot(true);
    }

    if (audioProcessor.guiState.syncingPosition.load(std::memory_order_acquire))
        syncPosition();

    if (audioProcessor.guiState.syncingField.load(std::memory_order_acquire))
        syncField();

    if (audioProcessor.getIRManager()->getDirectoryChanged().exchange(false, std::memory_order_acquire)) {
        if (settingsModal) settingsModal->refreshDirectories();
    }
        
}

// GUI state

void MareverbAudioProcessorEditor::updateIRSlot(bool animate) {
    int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    // DBG("Selected IR " << selectedIR);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        auto* irSlotButton = irSelectorPanel.getIRSlotButton(i);
        irSlotButton->setToggleState(i == selectedIR, juce::NotificationType::dontSendNotification);
    }

    selectedIRPanel.updateIRSlot(selectedIR, animate);
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
    std::vector<juce::Component*> positionControls = { &positionPatternControl, &positionRateControl, &positionModAControl, &positionModBControl};
    std::vector<juce::Component*> fieldControls = { &fieldPatternControl, &fieldModAControl, &fieldModBControl };

    auto selectPositionTab = [this, positionControls, fieldControls] {
        positionTabButton.setToggleState(true, juce::dontSendNotification);
        fieldTabButton.setToggleState(false, juce::dontSendNotification);
        for (auto* positionControl : positionControls) positionControl->setVisible(true);
        for (auto* fieldControl : fieldControls) fieldControl->setVisible(false);
        audioProcessor.guiState.syncingPosition.store(true);
    };

    auto selectFieldTab = [this, positionControls, fieldControls] {
        positionTabButton.setToggleState(false, juce::dontSendNotification);
        fieldTabButton.setToggleState(true, juce::dontSendNotification);
        for (auto* positionControl : positionControls) positionControl->setVisible(false);
        for (auto* fieldControl : fieldControls) fieldControl->setVisible(true);
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

void MareverbAudioProcessorEditor::initComponents() {
    // Weighting mode toggle switch
    // TODO: Change from TextButton to ToggleSwitch-esque component
    weightingModeControl.control.setClickingTogglesState(true);
    auto updateWeightingModeText = [this] {
        weightingModeControl.control.setButtonText(weightingModes[weightingModeControl.control.getToggleState()]);
    };
    updateWeightingModeText();
    weightingModeControl.control.onStateChange = updateWeightingModeText;

    topBarPanel.onSettingsClicked = [this]() {
        if (settingsModal) {
            settingsModal.reset();
        } else {
            settingsModal = std::make_unique<SettingsComponent>(audioProcessor.getIRManager(), animatorUpdater);
            settingsModal->onCloseRequested = [this]() { juce::MessageManager::callAsync([this]() { settingsModal.reset(); }); };
            settingsModal->setBounds(getLocalBounds().withSizeKeepingCentre(462, 361));
            addAndMakeVisible(*settingsModal);
        }
    };

    initPositionFieldControls();
}

// Layout

void MareverbAudioProcessorEditor::layoutLeftPanel(Bounds bounds) {
    Bounds mapBounds = bounds.removeFromTop(621);
    polarMapComponent.setBounds(mapBounds.reduced(static_cast<int>(mapBounds.getWidth() * 0.05f))); // Mare map

    layoutPositionFieldControls(bounds);
}

void MareverbAudioProcessorEditor::layoutRightPanel(Bounds bounds) {
    topBarPanel.setBounds(bounds.removeFromTop(81));
    irSelectorPanel.setBounds(bounds.removeFromTop(160));
    selectedIRPanel.setBounds(bounds.removeFromTop(280));
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

    topBarPanel(audioProcessor, animatorUpdater), 
    irSelectorPanel(audioProcessor, animatorUpdater), 
    selectedIRPanel(audioProcessor, animatorUpdater) {

    setLookAndFeel(&mainLookAndFeel);

    addChildComponent(valueTooltip);

    // PANELS
    addAndMakeVisible(topBarPanel);
    addAndMakeVisible(irSelectorPanel);
    addAndMakeVisible(selectedIRPanel);

    // COMPONENTS
    // TODO: Put these into MareMapPanel component class
    addAndMakeVisible(polarMapComponent);
    addAndMakeVisible(positionTabButton);
    addAndMakeVisible(fieldTabButton);

    // Setup base controls
    for (auto& control : controls) {
        addAndMakeVisible(*control.component);

        audioProcessor.apvts.addParameterListener(control.paramID, this);

        if (control.slider && control.paramID.isNotEmpty()) {
            // Bind control formatting to apvts parameter formatting
            if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(control.paramID))) {
                control.slider->textFromValueFunction = [param](double value) {
                    return param->getText(param->convertTo0to1(static_cast<float>(value)), 0);
                };
                control.slider->valueFromTextFunction = [param](const juce::String& text) {
                    return static_cast<double>(param->convertFrom0to1(param->getValueForText(text)));
                };
            }

            if (auto* valueTooltipClientSlider = dynamic_cast<ValueTooltipClient*>(control.slider))
                valueTooltipClientSlider->bindValueTooltipCallbacks(valueTooltip, *this);
        }
    }

    initComponents();

    startTimerHz(REFRESH_RATE);
    setSize(1046, 721);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() {
    // Detach look and feel
    setLookAndFeel(nullptr);

    // Detach listeners
    for (auto& control : controls) audioProcessor.apvts.removeParameterListener(control.paramID, this);
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
    // g.fillRect(topBarBounds.reduced(2));

    Bounds irGridBounds = rightPanel.removeFromTop(160);
    g.setColour(Theme::Colors::section);
    // g.fillRect(irGridBounds.reduced(2));

    Bounds irHeaderBounds = rightPanel.removeFromTop(40);
    // g.fillRect(irHeaderBounds.reduced(2));
    
    Bounds irWaveformBounds = rightPanel.removeFromTop(140);
    // g.setColour(Theme::Colors::background);
    // g.fillRect(irWaveformBounds.reduced(2));
    // g.setColour(juce::Colours::floralwhite.withAlpha(0.4f));
    // g.drawRoundedRectangle(irWaveformBounds.reduced(1).toFloat(), 4.0f, 0.8f);

    Bounds irControlsBounds = rightPanel.removeFromTop(100);
    // g.setColour(Theme::Colors::section);
    // g.fillRect(irControlsBounds.reduced(2));

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