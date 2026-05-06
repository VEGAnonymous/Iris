#include "GUI/Components/IRControlsComponent.h"
#include "GUI/Theme/Theme.h"

/* PRIVATE */

void IRControlsComponent::initIRButtons() {
    // IR buttons
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
}

void IRControlsComponent::initIRControls() {
    // IR controls
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        irControls[i] = std::make_unique<IRControl>(animatorUpdater);

        IRControl* irControl = nullptr;
        if (irControls[i]) {
            irControl = irControls[i].get();

            auto& gainRotary = irControl->gainControl.control;
            gainRotary.setNormalisableRange(juce::NormalisableRange<double>(-12.6, 12.6, 0.1)); // dB
            gainRotary.setDoubleClickReturnValue(true, 0.0, juce::ModifierKeys::noModifiers);

            gainRotary.onValueChange = [this, irControl]() {
                int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
                if (validateIRIndex(selectedIR)) {
                    const float nGainDB = static_cast<float>(irControl->gainControl.control.getValue());

                    auto slotTree = audioProcessor.apvts.state.getChildWithName(TreeID::irManagerState).getChild(selectedIR);
                    jassert(slotTree.isValid());
                    if (slotTree.isValid()) slotTree.setProperty(PropertyID::IRSlot::gainDB, nGainDB, nullptr);

                    IRCommand cmd = { IRCommand::IR_SET_GAIN_DB };
                    cmd.irIndex = selectedIR;
                    cmd.gainDB = nGainDB;
                    audioProcessor.getIRManager()->enqueueCommand(cmd);

                    audioProcessor.guiState.irControlsChanged.store(true, std::memory_order_release);
                }
            };

            gainRotary.textFromValueFunction = [this](double value) {
                if (abs(value) < EPSILON) return juce::String("0 dB");
                return Format::decibels(static_cast<float>(value), 4);
            };

            gainRotary.bindValueTooltipCallbacks(valueTooltip, parentComponent);
            gainRotary.setTooltip("The gain to apply to this IR.\nNote that this will be reset when a new IR is loaded into this slot, including via auto-swap.");
            addChildComponent(irControl->gainControl);
        }
    }
}

void IRControlsComponent::initEnvelopeControl() {
    // Envelope control
    envelopeControl.onEnvelopeChanged = [this](EnvelopeType type, float attack, float release) {
        int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
        if (validateIRIndex(selectedIR)) {
            auto slotTree = audioProcessor.apvts.state.getChildWithName(TreeID::irManagerState).getChild(selectedIR);
            jassert(slotTree.isValid());
            if (slotTree.isValid()) {
                slotTree.setProperty(PropertyID::IRSlot::Window::Envelope::type, static_cast<int>(type), nullptr);
                slotTree.setProperty(PropertyID::IRSlot::Window::Envelope::attack, attack, nullptr);
                slotTree.setProperty(PropertyID::IRSlot::Window::Envelope::release, release, nullptr);
            }

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

    // envelopeControl.bindValueTooltipCallbacks(valueTooltip, parentComponent);
    envelopeControl.setTooltip("Drag to set the envelope of the windowed IR range.\nRight-click to select the envelope type.\nDisclaimer: this has little practical use outside of potentially reducing windowing discontinuities.");
    addAndMakeVisible(envelopeControl);
}

void IRControlsComponent::initSwapControls() {
    // Swap controls
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        swapControls[i] = std::make_unique<SwapControl>(animatorUpdater);

        SwapControl* swapControl = nullptr;
        if (swapControls[i]) {
            swapControl = swapControls[i].get();

            auto& swapActiveToggle = swapControl->swapActiveToggle.control;
            auto& swapRangeSlider = swapControl->swapRangeSlider.control;

            swapActiveToggle.onStateChange = [this, &swapActiveToggle]() {
                int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
                if (validateIRIndex(selectedIR)) {
                    const bool nActiveState = swapActiveToggle.getToggleState();

                    auto slotTree = audioProcessor.apvts.state.getChildWithName(TreeID::irManagerState).getChild(selectedIR);
                    jassert(slotTree.isValid());
                    if (slotTree.isValid()) slotTree.setProperty(PropertyID::IRSlot::AutoSwap::swapActive, nActiveState, nullptr);
                    
                    updateSwapState(selectedIR, nActiveState);

                    IRCommand cmd = { IRCommand::IR_SET_SWAP_ACTIVE };
                    cmd.irIndex = selectedIR;
                    cmd.swapActiveState = nActiveState;
                    audioProcessor.getIRManager()->enqueueCommand(cmd);
                }
            };

            swapRangeSlider.onRangeChanged = [this](float min, float max) {
                int selectedIR = audioProcessor.apvts.state.getProperty(PropertyID::selectedIR);
                if (validateIRIndex(selectedIR)) {
                    const float minTime = juce::jmap(min, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX);
                    const float maxTime = juce::jmap(max, SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX);

                    auto slotTree = audioProcessor.apvts.state.getChildWithName(TreeID::irManagerState).getChild(selectedIR);
                    jassert(slotTree.isValid());
                    if (slotTree.isValid()) {
                        slotTree.setProperty(PropertyID::IRSlot::AutoSwap::swapMin, minTime, nullptr);
                        slotTree.setProperty(PropertyID::IRSlot::AutoSwap::swapMax, maxTime, nullptr);
                    }

                    IRCommand cmd = { IRCommand::IR_SET_SWAP_INTERVAL };
                    cmd.irIndex = selectedIR;
                    cmd.swapMinTime = minTime;
                    cmd.swapMaxTime = maxTime;
                    audioProcessor.getIRManager()->enqueueCommand(cmd);
                }
            };

            swapRangeSlider.formatTextFromValueFunction = [this](double value) {
                const float mappedValue = juce::jmap(static_cast<float>(value), SWAP_INTERVAL_MIN, SWAP_INTERVAL_MAX);
                return Format::seconds(mappedValue, 4);
            };

            swapRangeSlider.bindValueTooltipCallbacks(valueTooltip, parentComponent);
            swapControl->swapActiveToggle.control.setTooltip("Enables/disables auto-swap for this slot, which will load a random IR into this slot at random intervals.");
            swapRangeSlider.setTooltip("Drag the handles or drag-select to set the minimum and maximum time interval for auto-swapping.");
            addChildComponent(swapControl->swapActiveToggle);
            addChildComponent(swapControl->swapRangeSlider);
        }
    }
}

void IRControlsComponent::prepare() {
    initIRButtons();
    initIRControls();
    initEnvelopeControl();
    initSwapControls();
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
    // This layout is absolutely disgusting by the way
    Bounds bounds = getLocalBounds();

    // 'Random' / 'Clear' button
    juce::FlexBox irButtonColumn(juce::FlexBox::JustifyContent::center);
    irButtonColumn.flexDirection = juce::FlexBox::Direction::column;
    irButtonColumn.alignItems = juce::FlexBox::AlignItems::center;

    irButtonColumn.items.add(juce::FlexItem(randomIRButton)
        .withFlex(0.0f)
        .withWidth(62.1f)
        .withHeight(30.0f)
        .withMargin(juce::FlexItem::Margin(0.0f, 0.0f, 4.0f, 0.0f)));

    irButtonColumn.items.add(juce::FlexItem(clearIRButton)
        .withFlex(0.0f)
        .withWidth(62.1f)
        .withHeight(30.0f)
        .withMargin(juce::FlexItem::Margin(4.0f, 0.0f, 0.0f, 0.0f)));

    irButtonColumn.performLayout(bounds.removeFromLeft(100).reduced(12).withTrimmedLeft(6));

    bounds.removeFromLeft(20);

    // IR controls
    Bounds irControlColumn = bounds.removeFromLeft(116);
    Bounds irControlsBounds = irControlColumn.removeFromTop(56);

    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        auto& gainControl = irControls[i]->gainControl;

        gainControl.setBounds(irControlsBounds);
        gainControl.setLabelPosition(LabelledControl<Rotary>::LabelPosition::LEFT);
        gainControl.setLabelDimensions(40.0f, 12.0f);
        gainControl.setControlDimensions(50.0f, 50.0f);
        gainControl.setLabelMargin(juce::FlexItem::Margin(5.0f, 0.0f, 5.0f, 10.0f));
        gainControl.setControlMargin(juce::FlexItem::Margin(5.0f, 5.0f, 5.0f, 8.0f));
        gainControl.resized();
    }

    envelopeControl.setBounds(irControlColumn.withSizeKeepingCentre(116, 36).withTrimmedBottom(10));

    bounds.removeFromLeft(28);
    bounds.removeFromRight(20);

    // Swap controls
    Bounds swapToggleBounds = bounds.removeFromTop(34);
    Bounds swapSliderBounds = bounds;
    for (int i = 0; i < MAX_IR_COUNT; ++i) {
        auto& activeToggle = swapControls[i]->swapActiveToggle;
        auto& rangeSlider = swapControls[i]->swapRangeSlider;

        activeToggle.setBounds(swapToggleBounds.reduced(4));
        activeToggle.setLabelPosition(LabelledControl<HoverableToggleButton>::LabelPosition::LEFT);
        activeToggle.setLabelDimensions(66.0f, 12.0f);
        activeToggle.setControlDimensions(12.0f, 12.0f);
        activeToggle.setLabelMargin(juce::FlexItem::Margin(15.0f, 0.0f, 5.0f, 0.0f));
        activeToggle.setControlMargin(juce::FlexItem::Margin(15.0f, 5.0f, 5.0f, 8.0f));
        activeToggle.resized();

        rangeSlider.setBounds(swapSliderBounds.reduced(4));
        rangeSlider.setLabelDimensions(92.0f, 12.0f);
        rangeSlider.setControlDimensions(116.0f, 20.0f);
        rangeSlider.setLabelMargin(juce::FlexItem::Margin(0.0f, 0.0f, 6.0f, 0.0f));
        rangeSlider.setControlMargin(juce::FlexItem::Margin(5.0f, 0.0f, 10.0f, 0.0f));
        rangeSlider.resized();
    }
}

void IRControlsComponent::updateSwapState(int irIndex, bool nActiveState) {
    if (!validateIRIndex(irIndex)) return;

    swapControls[irIndex]->swapActiveToggle.control.setToggleState(nActiveState, juce::dontSendNotification);
    swapControls[irIndex]->swapRangeSlider.setEnabled(nActiveState);
    swapControls[irIndex]->swapRangeSlider.repaint();

    IRCommand cmd = { IRCommand::IR_SET_SWAP_ACTIVE };
    cmd.irIndex = irIndex;
    cmd.swapActiveState = nActiveState;
    audioProcessor.getIRManager()->enqueueCommand(cmd);
}

EnvelopeControl* IRControlsComponent::getEnvelopeControl() { return &envelopeControl; }

IRControlsComponent::IRControl* IRControlsComponent::getIRControl(int irIndex) {
    jassert(validateIRIndex(irIndex));
    return irControls[irIndex].get();
}

IRControlsComponent::SwapControl* IRControlsComponent::getSwapControl(int irIndex) { 
    jassert(validateIRIndex(irIndex));
    return swapControls[irIndex].get(); 
}