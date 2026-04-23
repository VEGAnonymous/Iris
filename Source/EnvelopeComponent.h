#pragma once

#include "ComboBoxLookAndFeel.h"
#include "Envelope.h"
#include "IRManager.h"
#include "Rotary.h"
#include "RotaryLookAndFeel.h"
#include "Theme.h"

#include <JuceHeader.h>

class EnvelopeComponent : public juce::Component {
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

    void drawCurve(juce::Graphics& g, juce::Rectangle<int> area);
    void updateCurve();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EnvelopeComponent)

public:
    std::function<void(EnvelopeType, float attackNorm, float releaseNorm)> onEnvelopeChanged;

    EnvelopeComponent();
    ~EnvelopeComponent() override;

    void setSlot(const IRSlot& slot);

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseMove(const juce::MouseEvent& e) override;
    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
};