#include "GUI/API/AnimatedValue.h"

/* PUBLIC */

AnimatedValue::AnimatedValue(juce::Component& base, juce::AnimatorUpdater& updater, int animateTimeMs) : animationUpdater(updater) {
    juce::Component::SafePointer<juce::Component> basePtr(&base);
    animator = std::make_unique<juce::Animator>(juce::ValueAnimatorBuilder{}
        .withDurationMs(animateTimeMs)
        .withEasing(juce::Easings::createEaseIn())
        .withValueChangedCallback([this, basePtr](float value) {
            if (!basePtr) return;
            currentValue = juce::jmap(value, currentValue, targetValue);
            basePtr->repaint();
        })
        .build());

    animationUpdater.addAnimator(*animator);
}

void AnimatedValue::setValue(float nValue, bool animate) {
    nValue = juce::jlimit(0.0f, 1.0f, nValue);
    if (!animate) {
        currentValue = nValue;
        return;
    }

    if (animator) {
        if (nValue == targetValue) return;
        targetValue = nValue;
        animator->start();
    }
}

float AnimatedValue::getValue() const { return currentValue; }