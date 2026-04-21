#pragma once

#include <JuceHeader.h>

class AnimatedAlpha {
private:
    float currentAlpha = 0.0f;
    float targetAlpha = 0.0f;

    juce::AnimatorUpdater& animationUpdater;
    std::unique_ptr<juce::Animator> animator;

public:
    AnimatedAlpha(juce::Component& base, juce::AnimatorUpdater& updater, int animateTimeMs = 75) : animationUpdater(updater) {
        juce::Component::SafePointer<juce::Component> basePtr(&base);
        animator = std::make_unique<juce::Animator>(juce::ValueAnimatorBuilder{}
            .withDurationMs(animateTimeMs)
            .withEasing(juce::Easings::createEaseIn())
            .withValueChangedCallback([this, basePtr](float value) {
                if (!basePtr) return;
                currentAlpha = juce::jmap(value, currentAlpha, targetAlpha);
                basePtr->repaint();
            })
            .build());

        animationUpdater.addAnimator(*animator);
    }

    /* // Causes an assertion failure for some reason
    ~AnimatedAlpha() {
        if (animatorIn) animationUpdater.removeAnimator(*animatorIn);
        if (animatorOut) animationUpdater.removeAnimator(*animatorOut);
    }
    */

    void setAlpha(float nAlpha, bool animate = true) { 
        nAlpha = juce::jlimit(0.0f, 1.0f, nAlpha);
        if (!animate) {
            currentAlpha = nAlpha; 
            return;
        }

        if (animator) {
            if (nAlpha == targetAlpha) return;
            targetAlpha = nAlpha;
            animator->start();
        }
    }

    float getAlpha() const { return currentAlpha; }
};