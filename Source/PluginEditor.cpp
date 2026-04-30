#include "Core/Utilities.h"
#include "Core/Control/MotionController.h"
#include "GUI/GUIUtilities.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Theme/Theme.h"
#include "GUI/API/ValueTooltipClient.h"
#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <array>

/* PRIVATE */

// Listeners and callbacks

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    if (parameterID == ParamID::positionPattern || parameterID == ParamID::positionModA || parameterID == ParamID::positionModB) {
        audioProcessor.guiState.positionPathChanged.store(true, std::memory_order_release);
        audioProcessor.guiState.updatePosition.store(true, std::memory_order_release);
        if (parameterID == ParamID::positionPattern)
            audioProcessor.guiState.syncingPosition.store(true, std::memory_order_release);
    }

    if (parameterID == ParamID::weightingMode || parameterID == ParamID::strength || parameterID == ParamID::spread) {
        audioProcessor.guiState.updateWeights.store(true, std::memory_order_release);
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

    // Polar map
    if (audioProcessor.guiState.positionPathChanged.exchange(false, std::memory_order_acquire))
        polarMapPanel.notifyPathChanged();

    if (audioProcessor.guiState.positionChanged.exchange(false, std::memory_order_acquire))
        polarMapPanel.notifyPositionChanged(audioProcessor.guiState.position.load(std::memory_order_relaxed));

    if (audioProcessor.guiState.fieldChanged.exchange(false, std::memory_order_acquire)) {
        std::vector<PolarCoordinate> coordinates;
        {
            juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.fieldLock);
            coordinates = audioProcessor.guiState.fieldCoordinates;
        }
        polarMapPanel.notifyFieldChanged(std::move(coordinates));
    }

    if (audioProcessor.guiState.syncingPosition.load(std::memory_order_acquire)) 
        syncPosition();

    if (audioProcessor.guiState.syncingField.load(std::memory_order_acquire)) 
        syncField();

    // IRs
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
        
    if (polarMapPanel.getIRSwitched().exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        updateIRSlot(true);
    }

    if (audioProcessor.guiState.swapChanged.exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingSwap.store(true, std::memory_order_release);
        for (int i = 0; i < MAX_IR_COUNT; ++i) selectedIRPanel.getIRControlsComponent()->updateSwapState(i);
        audioProcessor.guiState.syncingSwap.store(false, std::memory_order_release);
    }

    // Settings modal
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
    audioProcessor.guiState.updateField.store(true, std::memory_order_release);
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

// Initialization

void MareverbAudioProcessorEditor::prepare() {
    // Bind base control callbacks and attach listeners
    for (auto& control : controls) {
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

            // Bind value tooltip callbacks
            if (auto* valueTooltipClientSlider = dynamic_cast<ValueTooltipClient*>(control.slider))
                valueTooltipClientSlider->bindValueTooltipCallbacks(valueTooltip, *this);
        }

        audioProcessor.apvts.addParameterListener(control.paramID, this);
    }

    // Attach swap control listeners
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        audioProcessor.apvts.addParameterListener(ParamID::irSwapMin(i), this);
        audioProcessor.apvts.addParameterListener(ParamID::irSwapMax(i), this);
        audioProcessor.apvts.addParameterListener(ParamID::irSwapActive(i), this);
    }

    // Modals
    topBarPanel.onSettingsClicked = [this]() {
        if (settingsModal) {
            settingsModal.reset();
        } else {
            settingsModal = std::make_unique<SettingsComponent>(audioProcessor, animatorUpdater);
            settingsModal->onCloseRequested = [this]() { juce::MessageManager::callAsync([this]() { settingsModal.reset(); }); };
            settingsModal->setBounds(getLocalBounds().withSizeKeepingCentre(462, 361));
            addAndMakeVisible(*settingsModal);
        }
        repaint();
    };
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor(MareverbAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),

    // Attachments
    globalMixControlAttachment(audioProcessor.apvts, ParamID::globalMix, globalMixControl.control),
    decayControlAttachment(audioProcessor.apvts, ParamID::decay, decayControl.control),
    lowCutControlAttachment(audioProcessor.apvts, ParamID::lowCut, lowCutControl.control),
    highCutControlAttachment(audioProcessor.apvts, ParamID::highCut, highCutControl.control),

    weightingModeControlAttachment(audioProcessor.apvts, ParamID::weightingMode, weightingModeControl.control),
    strengthControlAttachment(audioProcessor.apvts, ParamID::strength, strengthControl.control),
    spreadControlAttachment(audioProcessor.apvts, ParamID::spread, spreadControl.control),

    positionPatternControlAttachment(audioProcessor.apvts, ParamID::positionPattern, positionPatternControl.control),
    positionRateControlAttachment(audioProcessor.apvts, ParamID::positionRate, positionRateControl.control),
    positionModAControlAttachment(audioProcessor.apvts, ParamID::positionModA, positionModAControl.control),
    positionModBControlAttachment(audioProcessor.apvts, ParamID::positionModB, positionModBControl.control),

    fieldPatternControlAttachment(audioProcessor.apvts, ParamID::fieldPattern, fieldPatternControl.control),
    fieldRateControlAttachment(audioProcessor.apvts, ParamID::fieldRate, fieldRateControl.control),
    fieldModAControlAttachment(audioProcessor.apvts, ParamID::fieldModA, fieldModAControl.control),
    fieldModBControlAttachment(audioProcessor.apvts, ParamID::fieldModB, fieldModBControl.control),

    // Panels
    polarMapPanel(audioProcessor, animatorUpdater),
    positionFieldControlsPanel(audioProcessor, animatorUpdater,
        std::array<PositionFieldControlsPanel::ControlTab, 2> {
            PositionFieldControlsPanel::ControlTab 
            { &positionPatternControl, { &positionRateControl, &positionModAControl, &positionModBControl } },

            PositionFieldControlsPanel::ControlTab
            { &fieldPatternControl, { &fieldRateControl, &fieldModAControl, &fieldModBControl } }
        }
    ),
    topBarPanel(audioProcessor, animatorUpdater), 
    irSelectorPanel(audioProcessor, animatorUpdater), 
    selectedIRPanel(audioProcessor, animatorUpdater, valueTooltip, *this),
    interactionControlsPanel(audioProcessor,
        InteractionControlsPanel::InteractionRow
        { &weightingModeControl, { &strengthControl, &spreadControl} }
    ),
    globalControlsPanel(
        GlobalControlsPanel::GlobalRow
        { { &lowCutControl, &highCutControl, &decayControl, &globalMixControl } }
    ) {

    // Init
    setLookAndFeel(&mainLookAndFeel);

    addChildComponent(valueTooltip);

    addAndMakeVisible(polarMapPanel);
    addAndMakeVisible(positionFieldControlsPanel);
    addAndMakeVisible(topBarPanel);
    addAndMakeVisible(irSelectorPanel);
    addAndMakeVisible(selectedIRPanel);
    addAndMakeVisible(interactionControlsPanel);
    addAndMakeVisible(globalControlsPanel);

    prepare();

    startTimerHz(REFRESH_RATE);
    setSize(1046, 721);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() {
    // Detach look and feel
    setLookAndFeel(nullptr);

    // Detach listeners
    for (auto& control : controls) audioProcessor.apvts.removeParameterListener(control.paramID, this);
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        audioProcessor.apvts.removeParameterListener(ParamID::irSwapMin(i), this);
        audioProcessor.apvts.removeParameterListener(ParamID::irSwapMax(i), this);
        audioProcessor.apvts.removeParameterListener(ParamID::irSwapActive(i), this);
    }
}

void MareverbAudioProcessorEditor::paint (juce::Graphics& g) {
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

    // Right panel
    Bounds& rightPanel = totalBounds;
    g.setColour(Theme::Colors::background);
    g.fillRect(rightPanel);
}

void MareverbAudioProcessorEditor::paintOverChildren(juce::Graphics& g) {
    if (!settingsModal) return;

    juce::Path path;
    path.addRectangle(getLocalBounds());
    path.addRoundedRectangle(settingsModal->getBounds().toFloat(), 12.0f);
    path.addRectangle(topBarPanel.getBounds().toFloat());
    path.setUsingNonZeroWinding(false);

    g.setColour(Theme::Colors::outline.withAlpha(0.35f));
    g.fillPath(path);
}

void MareverbAudioProcessorEditor::resized() {
    Bounds bounds = getLocalBounds();
    Bounds leftBounds = bounds.removeFromLeft(621);
    Bounds& rightBounds = bounds;

    // Left panel
    Bounds mapBounds = leftBounds.removeFromTop(621);
    polarMapPanel.setBounds(mapBounds.reduced(static_cast<int>(mapBounds.getWidth() * 0.05f)));
    positionFieldControlsPanel.setBounds(leftBounds.reduced(PANEL_INSET));

    // Right panel
    topBarPanel.setBounds(rightBounds.removeFromTop(81));
    irSelectorPanel.setBounds(rightBounds.removeFromTop(160).reduced(PANEL_INSET));
    selectedIRPanel.setBounds(rightBounds.removeFromTop(280));
    interactionControlsPanel.setBounds(rightBounds.removeFromTop(100).reduced(PANEL_INSET));
    globalControlsPanel.setBounds(rightBounds.removeFromTop(100).reduced(PANEL_INSET));
}