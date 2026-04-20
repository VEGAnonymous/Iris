#pragma once

#include <JuceHeader.h>

class AnimatedAlpha {
private:
    float animateAlpha = 0.0f;
    bool holdState = false;

    juce::AnimatorUpdater& animationUpdater;
    std::unique_ptr<juce::Animator> animatorIn, animatorOut;

public:
    AnimatedAlpha(juce::Component& base, juce::AnimatorUpdater& updater, bool hold = false, int animateTimeMs = 75)
        : animationUpdater(updater), holdState(hold) {
        juce::Component::SafePointer<juce::Component> basePtr(&base);
        animatorIn = std::make_unique<juce::Animator>(juce::ValueAnimatorBuilder{}
            .withDurationMs(animateTimeMs)
            .withEasing(juce::Easings::createEaseIn())
            .withValueChangedCallback([basePtr, this](float value) {
                if (basePtr) {
                    animateAlpha = value;
                    basePtr->repaint();

                    if (animatorIn->isComplete()) {
                        if (!basePtr->isMouseOver(true) && !holdState) {
                            animateOut();
                            return;
                        }
                    }
                }
            })
            .build());

        animatorOut = std::make_unique<juce::Animator>(juce::ValueAnimatorBuilder{}
            .withDurationMs(animateTimeMs)
            .withEasing(juce::Easings::createEaseIn())
            .withValueChangedCallback([basePtr, this](float value) {
                if (basePtr) {
                    animateAlpha = 1.0f - value;
                    basePtr->repaint();
                    if (animatorOut->isComplete()) {
                        if (basePtr->isMouseOver(true) && !holdState) {
                            animateIn();
                            return;
                        }
                    }
                }
            })
            .build());

        animationUpdater.addAnimator(*animatorIn);
        animationUpdater.addAnimator(*animatorOut);
    }

    /* // Causes an assertion failure for some reason
    ~AnimatedAlpha() {
        if (animatorIn) animationUpdater.removeAnimator(*animatorIn);
        if (animatorOut) animationUpdater.removeAnimator(*animatorOut);
    }
    */

    void animateIn() { if (animatorIn) animatorIn->start(); }
    void animateOut() { if (animatorOut) animatorOut->start(); }

    void setAnimateAlpha(float nAlpha) { animateAlpha = juce::jlimit(0.0f, 1.0f, nAlpha); } // Override animation
    float getAnimateAlpha() const { return animateAlpha; }
};