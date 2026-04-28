#pragma once

#include <JuceHeader.h>

class AnimatedAlpha {
private:
    float currentAlpha = 0.0f;
    float targetAlpha = 0.0f;

    juce::AnimatorUpdater& animationUpdater;
    std::unique_ptr<juce::Animator> animator;

public:
    AnimatedAlpha(juce::Component& base, juce::AnimatorUpdater& updater, int animateTimeMs = 75);
    
    /* // Causes an assertion failure for some reason
    ~AnimatedAlpha() {
        if (animatorIn) animationUpdater.removeAnimator(*animatorIn);
        if (animatorOut) animationUpdater.removeAnimator(*animatorOut);
    }
    */

    void setAlpha(float nAlpha, bool animate = true);
    float getAlpha() const;
};