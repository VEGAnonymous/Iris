#include "AnimatedAlpha.h"

/* PUBLIC */

AnimatedAlpha::AnimatedAlpha(juce::Component& base, juce::AnimatorUpdater& updater, int animateTimeMs) : animationUpdater(updater) {
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

void AnimatedAlpha::setAlpha(float nAlpha, bool animate) {
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

float AnimatedAlpha::getAlpha() const { return currentAlpha; }