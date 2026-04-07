#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "MotionController.h"
#include "Utilities.h"

/* PRIVATE */

void MareverbAudioProcessorEditor::parameterChanged(const juce::String& parameterID, float newValue) {
    if (parameterID == "Motion Pattern" || parameterID == "Motion Mod A" || parameterID == "Motion Mod B") {
        motionPatternChanged.set(true);
    }
}
void MareverbAudioProcessorEditor::timerCallback() {
    if (motionPatternChanged.exchange(false)) {
        // auto settings = getSettings(audioProcessor.apvts);
        repaint(); // <- Move to child component
    }

    if (audioProcessor.positionChanged.exchange(false, std::memory_order_acquire)) {
        currentPosition = audioProcessor.position.load(std::memory_order_relaxed);
        repaint();
    }
}

/* PUBLIC */

MareverbAudioProcessorEditor::MareverbAudioProcessorEditor (MareverbAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
    globalMixControlAttachment(audioProcessor.apvts, "Global Mix", globalMixControl),
    decayControlAttachment(audioProcessor.apvts, "Decay", decayControl),
    motionPatternControlAttachment(audioProcessor.apvts, "Motion Pattern", motionPatternControl),
    motionRateControlAttachment(audioProcessor.apvts, "Motion Rate", motionRateControl),
    motionModAControlAttachment(audioProcessor.apvts, "Motion Mod A", motionModAControl),
    motionModBControlAttachment(audioProcessor.apvts, "Motion Mod B", motionModBControl) {

    motionPatternControl.addItemList(motionPatterns, 1);

    // Attach listeners
    audioProcessor.apvts.addParameterListener("Global Mix", this);
    audioProcessor.apvts.addParameterListener("Decay", this);
    audioProcessor.apvts.addParameterListener("Motion Pattern", this);
    audioProcessor.apvts.addParameterListener("Motion Rate", this);
    audioProcessor.apvts.addParameterListener("Motion Mod A", this);
    audioProcessor.apvts.addParameterListener("Motion Mod B", this);

    for (auto* component : getComponents()) addAndMakeVisible(component);

    startTimerHz(REFRESH_RATE);
    setSize(405, 621);
}

MareverbAudioProcessorEditor::~MareverbAudioProcessorEditor() { 
    // Detach listeners
    audioProcessor.apvts.removeParameterListener("Global Mix", this);
    audioProcessor.apvts.removeParameterListener("Decay", this);
    audioProcessor.apvts.removeParameterListener("Motion Pattern", this);
    audioProcessor.apvts.removeParameterListener("Motion Rate", this);
    audioProcessor.apvts.removeParameterListener("Motion Mod A", this);
    audioProcessor.apvts.removeParameterListener("Motion Mod B", this);
}

// GUI

void MareverbAudioProcessorEditor::paint (juce::Graphics& g) {
    g.fillAll(juce::Colours::black);

    auto bounds = getLocalBounds();
    auto mareMapArea = bounds.removeFromTop(405);

    // auto w = mareMapArea.getWidth();

    g.setColour(juce::Colours::floralwhite);
    g.drawRoundedRectangle(mareMapArea.toFloat(), 4.0f, 1.0f);
    g.setOpacity(0.5f);
    g.drawEllipse(mareMapArea.toFloat(), 2.0f);

    const float boundMin = mareMapArea.toFloat().getX(), boundMax = mareMapArea.toFloat().getRight(); // X or Y works, it's a square
    auto map = [boundMin, boundMax](CartesianCoordinate p) {
        return CartesianCoordinate{
            juce::jmap(p.x, -1.0f, 1.0f, boundMin, boundMax),
            juce::jmap(p.y, -1.0f, 1.0f, boundMin, boundMax)
        };
    };

    // Draw parametric curve
    Settings settings = getSettings(audioProcessor.apvts);
    MotionPattern& pattern = settings.motionPattern;

    if (pattern != MotionPattern::RANDOM_DISCRETE && pattern != MotionPattern::RANDOM_WALK) {
        juce::Path parametricPath;

        float& motionRate = settings.motionRate, motionModA = settings.motionModA, motionModB = settings.motionModB;
        CartesianCoordinate initP = map(polarToCartesian(MotionController::computeParametric(pattern, motionModA, motionModB, 0.0f))); // t = 0.0f
        parametricPath.startNewSubPath(initP.x, initP.y);
        for (float t = 0.01f; t < 10.0f; t += 0.01f) {
            CartesianCoordinate p = map(polarToCartesian(MotionController::computeParametric(pattern, motionModA, motionModB, t)));
            parametricPath.lineTo(p.x, p.y);
        }

        g.setColour(juce::Colours::white);
        g.setOpacity(0.2f);
        g.strokePath(parametricPath, juce::PathStrokeType(2.0));
    }

    // Draw position indicator
    CartesianCoordinate currentPos = map(polarToCartesian(currentPosition));
    g.setColour(juce::Colours::lightcyan);
    g.setOpacity(1.0f);
    const float radius = 4.0f;
    g.fillEllipse(currentPos.x - radius, currentPos.y - radius, radius * 2.0f, radius * 2.0f);
}

void MareverbAudioProcessorEditor::resized() {
    // Window layout
    auto bounds = getLocalBounds();

    // Map
    auto mareMapArea = bounds.removeFromTop(405);

    // Parameter rows
    juce::FlexBox motionControlRow;
    std::vector<juce::Component*> motionControls = { &motionPatternControl, &motionRateControl, &motionModAControl, &motionModBControl };
    for (auto* component : motionControls) 
        motionControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    juce::FlexBox globalControlRow;
    std::vector<juce::Component*> globalControls = { &globalMixControl, &decayControl };
    for (auto* component : globalControls) 
        globalControlRow.items.add(juce::FlexItem(*component).withFlex(1.0f).withMaxHeight(90));

    // Main parameter column
    juce::FlexBox uiColumn;
    uiColumn.flexDirection = juce::FlexBox::Direction::column;
    uiColumn.items.add(juce::FlexItem(motionControlRow).withFlex(1.0f));
    uiColumn.items.add(juce::FlexItem(globalControlRow).withFlex(1.0f));
    uiColumn.performLayout(bounds);
}

std::vector<juce::Component*> MareverbAudioProcessorEditor::getComponents() {
    return { &globalMixControl, &decayControl, &motionPatternControl, &motionRateControl, &motionModAControl, &motionModBControl };
}