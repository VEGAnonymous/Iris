#include "GUI/Components/IRControlsComponent.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void IRControlsComponent::prepare() {
    clearIRButton.setButtonText("CLEAR");
    clearIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) {
            IRCommand cmd = { IRCommand::CommandType::IR_CLEAR };
            cmd.irIndex = selectedIR;
            audioProcessor.getIRManager()->enqueueCommand(cmd);
        }
    };
    clearIRButton.setTooltip("Clears the selected IR slot.");
    addAndMakeVisible(clearIRButton);

    randomIRButton.setButtonText("RANDOM");
    randomIRButton.onClick = [this]() {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) {
            IRCommand cmd = { IRCommand::CommandType::IR_LOAD_RANDOM };
            cmd.irIndex = selectedIR;
            audioProcessor.getIRManager()->enqueueCommand(cmd);
        }
    };
    randomIRButton.setTooltip("Loads a random IR from the supplied directory list.\nAdjust settings via the directory manager in the top bar.");
    addAndMakeVisible(randomIRButton);

    // Envelope control
    envelopeControl.onEnvelopeChanged = [this](EnvelopeType type, float attack, float release) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) {
            IRCommand cmd = { IRCommand::IR_SET_ENVELOPE };
            cmd.irIndex = selectedIR;
            cmd.envelopeType = type;
            cmd.envelopeAttack = attack;
            cmd.envelopeRelease = release;
            audioProcessor.getIRManager()->enqueueCommand(cmd);
        }
    };

    envelopeControl.formatTextFromValueFunction = [this](double value) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        const auto& slot = audioProcessor.getIRManager()->getIRSlot(selectedIR);
        double windowDur = (slot.window.length * slot.numSamples) / audioProcessor.getSampleRate();
        double valueDur = juce::jmap(value, 0.0, windowDur);
        return Format::seconds(static_cast<float>(valueDur), 4);
    };

    envelopeControl.bindValueTooltipCallbacks(valueTooltip, parentComponent);

    envelopeControl.setTooltip("Drag to set the envelope of the windowed IR range.\nDisclaimer: this has little practical use outside of potentially reducing windowing discontinuities.");

    addAndMakeVisible(envelopeControl);

    // Swap controls
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        swapControls[i] = std::make_unique<SwapControl>(audioProcessor.apvts, animatorUpdater, i);
        
        swapControls[i]->swapActiveToggle.control.setTooltip("Enables/disables auto-swap for this slot, which will load a random IR into this slot at random intervals.");
        swapControls[i]->swapRangeSlider.control.setTooltip("Drag the handles or drag-select to set the minimum and maximum time interval for auto-swapping.");

        addChildComponent(swapControls[i]->swapActiveToggle);
        addChildComponent(swapControls[i]->swapRangeSlider);

        // Bind control formatting to apvts parameter formatting
        if (auto* param = dynamic_cast<juce::RangedAudioParameter*>(audioProcessor.apvts.getParameter(ParamID::irSwapMin(i)))) {
            swapControls[i]->swapRangeSlider.control.formatTextFromValueFunction = [param, i](double value) {
                return param->getText(static_cast<float>(value), 0);
            };
        }

        swapControls[i]->swapRangeSlider.control.bindValueTooltipCallbacks(valueTooltip, parentComponent);
    }
}

/* PUBLIC */

IRControlsComponent::IRControlsComponent(MareverbAudioProcessor& processor, juce::AnimatorUpdater& updater, ValueTooltipWindow& tooltip, juce::Component& parent)
    : audioProcessor(processor), animatorUpdater(updater), valueTooltip(tooltip), parentComponent(parent) {
    prepare();
}

void IRControlsComponent::paint(juce::Graphics& g) {
	g.setColour(Theme::Colors::section);
	g.fillRoundedRectangle(getLocalBounds().toFloat(), PANEL_CORNER_SIZE);
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
        .withFlex(0.0f)
        .withWidth(w * 0.15f)
        .withHeight(h * 0.3f)
        .withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 4.0f, 0.0f)));

    irControlColumn.items.add(juce::FlexItem(clearIRButton)
        .withFlex(0.0f)
        .withWidth(w * 0.15f)
        .withHeight(h * 0.3f)
        .withMargin(juce::FlexItem::Margin(4.0f, 0.0f, 0.0f, 0.0f)));

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

void IRControlsComponent::updateSwapState(int irIndex) {
    if (!validateIRIndex(irIndex)) return;
    bool swapActive = swapControls[irIndex]->swapActiveToggle.control.getToggleState();
    swapControls[irIndex]->swapRangeSlider.setEnabled(swapActive);
    swapControls[irIndex]->swapRangeSlider.repaint();
    IRCommand cmd = { IRCommand::CommandType::IR_CLEAR };
    cmd.irIndex = irIndex;
    cmd.swapActiveState = swapActive;
    audioProcessor.getIRManager()->setSwapActive(irIndex, swapActive);
}

EnvelopeControl* IRControlsComponent::getEnvelopeControl() { return &envelopeControl; }
IRControlsComponent::SwapControl* IRControlsComponent::getSwapControl(int irIndex) { 
    jassert(validateIRIndex(irIndex));
    return swapControls[irIndex].get(); 
}