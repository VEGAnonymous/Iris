#include "Core/Utilities.h"
#include "Core/Control/MotionController.h"
#include "GUI/GUIUtilities.h"
#include "GUI/Components/Controls/LabelledControl.h"
#include "GUI/Theme/Theme.h"
#include "GUI/API/MareAlert.h"
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

    if (audioProcessor.guiState.syncingPosition.load(std::memory_order_acquire)) {
        syncPosition();
    }

    if (audioProcessor.guiState.syncingField.load(std::memory_order_acquire)) {
        syncField();
    }
        
    if (audioProcessor.guiState.indicatorStyleChanged.exchange(false, std::memory_order_acquire)) {
        syncIndicatorStyles();
    }

    // IRs
    if (audioProcessor.guiState.irChanged.exchange(false, std::memory_order_acquire)) {
        syncIRs(true);
    }

    if (audioProcessor.guiState.selectedIRChanged.exchange(false, std::memory_order_acquire)) {
        updateIRSlot(false);
    }
        
    if (polarMapPanel.getIRSwitched().exchange(false, std::memory_order_acquire)) {
        audioProcessor.guiState.syncingField.store(true, std::memory_order_release);
        updateIRSlot(true);
    }

    if (audioProcessor.getIRManager()->getIRLoaded().exchange(false, std::memory_order_acquire)) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        auto slotTree = audioProcessor.apvts.state.getChildWithName(TreeID::irManagerState).getChild(selectedIR);
        slotTree.setProperty(PropertyID::IRSlot::gainDB, 0.0f, nullptr);

        auto* irControl = selectedIRPanel.getIRControlsComponent()->getIRControl(selectedIR);
        jassert(irControl);
        if (irControl) irControl->gainControl.control.setValue(0.0f, juce::dontSendNotification);

        syncIRControls();
    }

    if (audioProcessor.guiState.irControlsChanged.exchange(false, std::memory_order_acquire)) {
        syncIRControls();
    }

    // Directory manager modal
    if (audioProcessor.getIRManager()->getDirectoryChanged().exchange(false, std::memory_order_acquire)) {
        if (directoryManagerModal) directoryManagerModal->refresh();
    }

    // Misc
    if (audioProcessor.guiState.tooltipsEnabledChanged.exchange(false, std::memory_order_acquire)) {
        syncSettings();
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

    selectedIRPanel.updateIRSlot(animate);
    audioProcessor.guiState.updateField.store(true, std::memory_order_release);
    DBG("SYNC: IR slot updated");
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
            positionModAControl.control.setTooltip("The radius of the polar coordinate position.\nAlternatively, set the position by clicking and dragging the indicator.");
            positionModBControl.control.setTooltip("The angle of the polar coordinate position.\nAlternatively, set the position by clicking and dragging the indicator.");
            positionRateControl.setEnabled(false);

            PolarCoordinate position = audioProcessor.guiState.position.load();
            nModA = positionModA->convertTo0to1(position.r);
            nModB = positionModB->convertTo0to1(position.theta / juce::MathConstants<float>::twoPi);
            break;
        }
        case PositionPattern::EYES: {
            positionModAControl.label.setText("Angle", juce::dontSendNotification);
            positionModBControl.label.setText("-", juce::dontSendNotification);
            positionModAControl.control.setTooltip("The derpiness of the eyes.");
            positionModBControl.control.setTooltip("");
            positionModBControl.setEnabled(false);
            break;
        }
        case PositionPattern::ORBIT: {
            positionModAControl.label.setText("Radius", juce::dontSendNotification);
            positionModBControl.label.setText("-", juce::dontSendNotification);
            positionModAControl.control.setTooltip("The radius of the circle to orbit.");
            positionModBControl.control.setTooltip("");
            positionModBControl.setEnabled(false);
            break;
        }
        case PositionPattern::SPIRAL: {
            positionModAControl.label.setText("Swirl", juce::dontSendNotification);
            positionModBControl.label.setText("-", juce::dontSendNotification);
            positionModBControl.setEnabled(false);
            positionModAControl.control.setTooltip("The swirliness of the swirl swirl.");
            positionModBControl.control.setTooltip("");
            break;
        }
        case PositionPattern::FLORAL:
        case PositionPattern::LISSAJOUS: {
            positionModAControl.label.setText("P", juce::dontSendNotification);
            positionModBControl.label.setText("Q", juce::dontSendNotification);
            positionModAControl.control.setTooltip("I wonder what this does?");
            positionModBControl.control.setTooltip("I wonder what this does?");
            break;
        }
        case PositionPattern::RANDOM_DISCRETE: {
            positionModAControl.label.setText("Radius", juce::dontSendNotification);
            positionModBControl.label.setText("Smoothing", juce::dontSendNotification);
            positionModAControl.control.setTooltip("The maximum range of movement away from the center.");
            positionModBControl.control.setTooltip("How smooth (and slow!) the journey to the target position is.");
            break;
        }
        case PositionPattern::RANDOM_WALK: {
            positionModAControl.label.setText("Wander", juce::dontSendNotification);
            positionModBControl.label.setText("Bounce", juce::dontSendNotification);
            positionModAControl.control.setTooltip("The \"impulsiveness\" of the random walk.\nHigher values create more energetic, jittery movement with frequent changes in speed and direction.");
            positionModBControl.control.setTooltip("How much the indicator will bounce back when it hits the edge.");
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
    DBG("SYNC: Position sync complete");
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

    // Set label text, tooltip, enabled/disabled for each pattern
    switch (fieldPattern) {
        case FieldPattern::MANUAL: {
            fieldRateControl.setEnabled(false);
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Angle", juce::dontSendNotification);
            fieldModAControl.control.setTooltip("The radius of the selected indicator's polar coordinate position.\nAlternatively, set the position by clicking and dragging the indicator.");
            fieldModBControl.control.setTooltip("The angle of the selected indicator's polar coordinate position.\nAlternatively, set the position by clicking and dragging the indicator.");

            PolarCoordinate coordinate {};
            {
                juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.fieldLock);
                int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
                if (validateIRIndex(selectedIR)) coordinate = audioProcessor.guiState.fieldCoordinates[selectedIR];
            }
            nModA = fieldModA->convertTo0to1(coordinate.r);
            nModB = fieldModB->convertTo0to1(coordinate.theta / juce::MathConstants<float>::twoPi);
            break;
        }
        case FieldPattern::RING: {
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Offset", juce::dontSendNotification);
            fieldModAControl.control.setTooltip("The radius of the circle to orbit.");
            fieldModBControl.control.setTooltip("The phase shift of the orbit as a whole.");
            break;
        }
        case FieldPattern::ORBITS: {
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Bias", juce::dontSendNotification);
            fieldModAControl.control.setTooltip("The radius of the outermost orbit.");
            fieldModBControl.control.setTooltip("The variation in orbital velocities between inner and outer orbits.\nHigher values bias velocity towards outer orbits, and vice versa.");
            break;
        }
        case FieldPattern::RANDOM_DISCRETE: {
            fieldModAControl.label.setText("Radius", juce::dontSendNotification);
            fieldModBControl.label.setText("Smoothing", juce::dontSendNotification);
            fieldModAControl.control.setTooltip("The maximum range of movement away from the center.");
            fieldModBControl.control.setTooltip("How smooth (and slow!) the journey to the target position is.");
            break;
        }
        case FieldPattern::RANDOM_WALK: {
            fieldModAControl.label.setText("Wander", juce::dontSendNotification);
            fieldModBControl.label.setText("Bounce", juce::dontSendNotification);
            fieldModAControl.control.setTooltip("The \"impulsiveness\" of the random walk.\nHigher values create more energetic, jittery movement with frequent changes in speed and direction.");
            fieldModBControl.control.setTooltip("How much indicators will bounce back when they hit the edge.");
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
    DBG("SYNC: Field sync complete");
}

void MareverbAudioProcessorEditor::syncIRs(bool animate) {
    // TODO: Make this selective instead of all at once
    {
        juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.irWaveformLock);
        for (int i = 0; i < MAX_IR_COUNT; ++i) {
            const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);

            // audioProcessor.apvts.getParameter(ParamID::irGain(i))->setValueNotifyingHost(slot.gain);

            const auto& waveform = audioProcessor.guiState.irWaveforms[i];
            auto* slotButton = irSelectorPanel.getIRSlotButton(i);

            slotButton->setOccupied(slot.occupied);
            slotButton->setActive(slot.active, animate);

            auto* waveformComponent = slotButton->getWaveformComponent();
            waveformComponent->setWaveform(slot.occupied ? waveform.get() : nullptr, audioProcessor.getSampleRate());
            waveformComponent->setGain(slot.gain);
        }
    }

    updateIRSlot(animate);
    DBG("SYNC: IR sync complete");
}

void MareverbAudioProcessorEditor::syncIRControls() {
    int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
    const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);

    // Sync gains
    auto* slotButton = irSelectorPanel.getIRSlotButton(selectedIR);
    auto* waveformComponent = slotButton->getWaveformComponent();
    if (waveformComponent) waveformComponent->setGain(slot.gain);

    waveformComponent = selectedIRPanel.getIRDisplayComponent()->getWaveformComponent();
    jassert(waveformComponent);
    if (waveformComponent) waveformComponent->setGain(slot.gain);
}

void MareverbAudioProcessorEditor::syncSwap() {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(i);
        selectedIRPanel.getIRControlsComponent()->updateSwapState(i, slot.autoSwap.active);
    }
    DBG("SYNC: Swap sync complete");
}

void MareverbAudioProcessorEditor::syncIndicatorStyles() {
    polarMapPanel.notifyIndicatorStyleChanged();
    irSelectorPanel.updateIndicatorStyle();
    selectedIRPanel.updateIndicatorStyle();

    audioProcessor.storePersistentState();

    DBG("SYNC: Updated indicator styles");
}

void MareverbAudioProcessorEditor::syncSettings() {
    const bool tooltipsEnabled = audioProcessor.apvts.state.getProperty(PropertyID::tooltipsEnabled, true);
    tooltipWindow.setAlpha(tooltipsEnabled ? 1.0f : 0.0f);
    tooltipWindow.setMillisecondsBeforeTipAppears(tooltipsEnabled ? INFO_TOOLTIP_TIME_MS : INT_MAX);
    DBG("SYNC: Set tooltips enabled: " << static_cast<int>(tooltipsEnabled));

    const int controlRateIndex = audioProcessor.apvts.state.getProperty(PropertyID::controlRate, 3);
    const float controlRate = controlRates[controlRateIndex - 1].removeCharacters(" Hz").getFloatValue();
    audioProcessor.setControlRate(controlRate);

    audioProcessor.storePersistentState();

    DBG("SYNC: Settings sync complete");
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

    // Modals
    topBarPanel.onDirectoryManagerClicked = [this]() {
        if (settingsModal) settingsModal.reset();
        if (directoryManagerModal) directoryManagerModal.reset();
        else {
            directoryManagerModal = std::make_unique<DirectoryManagerComponent>(audioProcessor, animatorUpdater);
            directoryManagerModal->onCloseRequested = [this]() { juce::MessageManager::callAsync([this]() { directoryManagerModal.reset(); }); };
            directoryManagerModal->setBounds(getLocalBounds().withSizeKeepingCentre(462, 480));
            addAndMakeVisible(*directoryManagerModal);
        }
        repaint();
    };

    topBarPanel.onSettingsClicked = [this]() {
        if (directoryManagerModal) directoryManagerModal.reset();
        if (settingsModal) settingsModal.reset();
        else {
            settingsModal = std::make_unique<SettingsComponent>(audioProcessor, animatorUpdater);
            settingsModal->onCloseRequested = [this]() { juce::MessageManager::callAsync([this]() { settingsModal.reset(); }); };
            settingsModal->setBounds(getLocalBounds().withSizeKeepingCentre(346, 444));
            addAndMakeVisible(*settingsModal);
        }
        repaint();
    };

    audioProcessor.getIRManager()->onAlert = [this](juce::String title, juce::String message, juce::String details, juce::String buttonText) {
        MareAlert::show(animatorUpdater, &mainLookAndFeel, title, message, details, buttonText);
    };

    // Set tooltips
    tooltipWindow.setAlwaysOnTop(true);
    globalMixControl.control.setTooltip("The blend between the dry input signal and the wet mares signal.");
    decayControl.control.setTooltip("The reverb decay as a percentage of the current longest active IR's duration.");
    lowCutControl.control.setTooltip("The cutoff frequency of the 12 dB/oct low-cut filter applied to the wet signal.");
    highCutControl.control.setTooltip("The cutoff frequency of the 12 dB/oct high-cut filter applied to the wet signal.");

    weightingModeControl.control.setTooltip("The method for blending IRs based on distance.\n\nAbsolute mode weights based on the true distance from the position indicator to any given active field indicator, independent of the others.\n\nRelative mode weights proportionally based on the relative distances from the position indicator to each other active field indicator.");
    strengthControl.control.setTooltip("The amount that distance affects each IR's contribution.\nHigher values emphasize nearby IRs and reduce the influence of distant ones.");
    spreadControl.control.setTooltip("The stereo \"width\" of the reverb, applied via weak binaural panning cues based on the angles between the position indicator and each active field indicator.");

    positionPatternControl.control.setTooltip("The motion pattern to use for the position indicator.");
    positionRateControl.control.setTooltip("The rate and direction of motion for the position indicator.\nNote that this parameter is asynchronous and thus has no consistent units.");

    fieldPatternControl.control.setTooltip("The motion pattern to use for the field indicators.");
    fieldRateControl.control.setTooltip("The rate and direction of motion for the field indicators.\nNote that this parameter is asynchronous and thus has no consistent units.");
    
    // Ponies, at the ready!
    initState();
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor(MareverbAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p), tooltipWindow(nullptr, INFO_TOOLTIP_TIME_MS),

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

    tooltipWindow.setLookAndFeel(&mainLookAndFeel);
    addChildComponent(valueTooltip);

    addAndMakeVisible(polarMapPanel);
    addAndMakeVisible(positionFieldControlsPanel);
    addAndMakeVisible(topBarPanel);
    addAndMakeVisible(irSelectorPanel);
    addAndMakeVisible(selectedIRPanel);
    addAndMakeVisible(interactionControlsPanel);
    addAndMakeVisible(globalControlsPanel);

    setVisible(false);
    prepare();
    setSize(1046, 721);
    setVisible(true);
    startTimerHz(REFRESH_RATE);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() {
    // Detach look and feel
    setLookAndFeel(nullptr);
    tooltipWindow.setLookAndFeel(nullptr);

    // Detach listeners
    for (auto& control : controls) audioProcessor.apvts.removeParameterListener(control.paramID, this);
}

void MareverbAudioProcessorEditor::paint(juce::Graphics& g) {
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
    if (!directoryManagerModal && !settingsModal) return;
    juce::Path path;
    path.addRectangle(getLocalBounds());

    BoundsF modalBounds {};
    if (directoryManagerModal) modalBounds = directoryManagerModal->getBounds().toFloat();
    else if (settingsModal) modalBounds = settingsModal->getBounds().toFloat();

    path.addRoundedRectangle(modalBounds, 12.0f);
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

void MareverbAudioProcessorEditor::initState() {
    syncPosition();
    syncField();
    syncIRs(false);
    syncIRControls();
    syncSwap();
    syncSettings();
    syncIndicatorStyles();

    polarMapPanel.notifyPathChanged();
    polarMapPanel.notifyPositionChanged(audioProcessor.guiState.position.load(std::memory_order_relaxed));
    {
        std::vector<PolarCoordinate> coordinates;
        juce::SpinLock::ScopedLockType lock(audioProcessor.guiState.fieldLock);
        coordinates = audioProcessor.guiState.fieldCoordinates;
        polarMapPanel.notifyFieldChanged(std::move(coordinates), false);
    }

    DBG("INIT: Init editor state");
}