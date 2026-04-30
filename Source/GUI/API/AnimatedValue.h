#pragma once

#include <JuceHeader.h>

class AnimatedValue {
private:
    float currentValue = 0.0f;
    float targetValue = 0.0f;

    juce::AnimatorUpdater& animationUpdater;
    std::unique_ptr<juce::Animator> animator;

public:
    AnimatedValue(juce::Component& base, juce::AnimatorUpdater& updater, int animateTimeMs = 75);
    
    /* // Causes an assertion failure for some reason
    ~AnimatedValue() {
        if (animatorIn) animationUpdater.removeAnimator(*animatorIn);
        if (animatorOut) animationUpdater.removeAnimator(*animatorOut);
    }
    */

    void setValue(float nValue, bool animate = true);
    float getValue() const;
};