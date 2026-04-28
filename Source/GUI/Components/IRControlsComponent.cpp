#include "GUI/Components/IRControlsComponent.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void IRControlsComponent::prepare() {
    addAndMakeVisible(valueTooltip);

    clearIRButton.setButtonText("CLEAR");
    clearIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) audioProcessor.getIRManager()->clearIR(selectedIR);
    };
    addAndMakeVisible(clearIRButton);

    randomIRButton.setButtonText("RANDOM");
    randomIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) audioProcessor.getIRManager()->loadRandomIR(selectedIR);
    };
    addAndMakeVisible(randomIRButton);

    // Envelope control
    envelopeControl.onEnvelopeChanged = [this](EnvelopeType type, float atk, float rel) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) audioProcessor.getIRManager()->setEnvelope(selectedIR, type, atk, rel);
    };

    envelopeControl.formatTextFromValueFunction = [this](double value) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
        double windowDur = (slot.window.length * slot.buffer.getNumSamples()) / audioProcessor.getSampleRate();
        double valueDur = juce::jmap(value, 0.0, windowDur);
        return Format::seconds(static_cast<float>(valueDur), 4);
    };

    envelopeControl.bindValueTooltipCallbacks(valueTooltip, *this);
    addAndMakeVisible(envelopeControl);

    // Swap controls
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        swapControls[i] = std::make_unique<SwapControl>(audioProcessor.apvts, animatorUpdater, i);
        addChildComponent(swapControls[i]->swapActiveToggle);
        addChildComponent(swapControls[i]->swapRangeSlider);

        audioProcessor.apvts.addParameterListener(ParamID::irSwapMin(i), this);
        audioProcessor.apvts.addParameterListener(ParamID::irSwapMax(i), this);
        audioProcessor.apvts.addParameterListener(ParamID::irSwapActive(i), this);

        // Bind control formatting to apvts parameter formatting
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(ParamID::irSwapMin(i)))) {
            swapControls[i]->swapRangeSlider.control.formatTextFromValueFunction = [param, i](double value) {
                return param->getText(static_cast<float>(value), 0);
            };
        }

        swapControls[i]->swapRangeSlider.control.bindValueTooltipCallbacks(valueTooltip, *this);
    }

    startTimerHz(REFRESH_RATE);
}

void IRControlsComponent::parameterChanged(const juce::String& parameterID, float /*newValue*/) {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        if (parameterID == ParamID::irSwapActive(i) /*|| parameterID == ParamID::irSwapMin(i) || parameterID == ParamID::irSwapMax(i)*/) {
            audioProcessor.guiState.swapChanged.store(true, std::memory_order_release);
        }
    }
}

void IRControlsComponent::timerCallback() {
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
}

/* PUBLIC */

IRControlsComponent::IRControlsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater)
    : audioProcessor(processor), animatorUpdater(updater) {
    prepare();
}

IRControlsComponent::~IRControlsComponent() {
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        audioProcessor.apvts.removeParameterListener(ParamID::irSwapMin(i), this);
        audioProcessor.apvts.removeParameterListener(ParamID::irSwapMax(i), this);
        audioProcessor.apvts.removeParameterListener(ParamID::irSwapActive(i), this);
    }
}

void IRControlsComponent::paint(juce::Graphics& g) {
	g.setColour(Theme::Colors::section);
	g.fillRect(getLocalBounds().reduced(2));
}

void IRControlsComponent::resized() {
    Bounds bounds = getLocalBounds();
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

    irControlRow.items.add(juce::FlexItem(envelopeControl) // Window envelope
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

EnvelopeControl* IRControlsComponent::getEnvelopeControl() { return &envelopeControl; }
IRControlsComponent::SwapControl* IRControlsComponent::getSwapControl(int irIndex) { 
    jassert(validateIRIndex(irIndex));
    return swapControls[irIndex].get(); 
}