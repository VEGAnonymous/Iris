#pragma once

#include "Core/Control/State/Envelope.h"
#include "Core/Control/IRManager.h"
#include "GUI/API/ValueTooltipClient.h"
#include "GUI/Theme/Theme.h"
#include "GUI/Theme/LookAndFeel/ComboBoxLookAndFeel.h"

#include <JuceHeader.h>

class EnvelopeControl : public juce::Component, public juce::SettableTooltipClient, public ValueTooltipClient {
private:
    Envelope envelope;
    Bounds curveBounds;
    std::vector<float> previewCurve; // downsampled

    juce::ComboBox typeSelector;
    ComboBoxLookAndFeel comboBoxLookAndFeel;

    enum class DragTarget { NONE, ATTACK, RELEASE };
    DragTarget dragTarget { DragTarget::NONE };
    float dragStartValue = 0.0f;

    bool updatingSlot = false;

    float map(float x) const;

    DragTarget hitSection(float x) const;

    void drawCurve(juce::Graphics& g, juce::Rectangle<int> area);
    void updateCurve();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeControl)

public:
    std::function<void(EnvelopeType, float attackNorm, float releaseNorm)> onEnvelopeChanged;

    EnvelopeControl();
    ~EnvelopeControl() override;

    void setSlot(const IRSlot& slot);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;

    juce::String getValueTooltip() override;
};