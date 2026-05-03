#include "GUI/Components/Controls/HoverableTextEditor.h"

/* PUBLIC */

HoverableTextEditor::HoverableTextEditor(juce::AnimatorUpdater& updater) : juce::TextEditor(), hoverAnim(*this, updater) {}

void HoverableTextEditor::mouseEnter(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(1.0f); }
void HoverableTextEditor::mouseExit(const juce::MouseEvent& /*e*/) { hoverAnim.setValue(0.0f); }

void HoverableTextEditor::mouseDown(const juce::MouseEvent& e) {
    juce::TextEditor::mouseDown(e);
    hoverAnim.setValue(0.0f);
}

void HoverableTextEditor::mouseUp(const juce::MouseEvent& e) {
    juce::TextEditor::mouseUp(e);
    hoverAnim.setValue(1.0f);
}

float HoverableTextEditor::getHoverAlpha() const { return hoverAnim.getValue(); }