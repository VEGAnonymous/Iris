#pragma once

#include <JuceHeader.h>

/* DEFINES */

// Constants
static constexpr auto EPSILON = 1e-3f,
                      sqrt2_2 = juce::MathConstants<float>::sqrt2 / 2.0f;       

// Convolution
static constexpr auto L = 1024,
                      PARTITION_SIZE = 2 * L,
                      FFT_SIZE = 4 * L, // 4096
                      FFT_ORDER = 12, // 2^12, hardcoded
                      HOP_SIZE = FFT_SIZE / 4, // L
                      OUTPUT_SIZE = 4 * L;

// Meta
static constexpr auto N_CHANNELS = 2,
                      MAX_IR_COUNT = 8,
                      MAX_IR_PARTITIONS = 512,
                      MAX_IR_SAMPLES = MAX_IR_PARTITIONS * 512, // ~6s @ 44.1k
                      MAX_IR_FILE_SAMPLES = MAX_IR_SAMPLES * 10; // ~60s @ 44.1k

static constexpr auto SWAP_INTERVAL_MIN = 6.2f,
                      SWAP_INTERVAL_MAX = 62.1f;

// Control
// static constexpr auto CONTROL_RATE = 30.0f;

// GUI
static constexpr auto REFRESH_RATE = 60;

static constexpr auto PANEL_INSET = 1.826f;
static constexpr auto PANEL_CORNER_SIZE = 4.0f;

static constexpr auto WAVEFORM_PREVIEW_POINTS = 126,
                      WAVEFORM_POINTS = 216,
                      ENVELOPE_PREVIEW_POINTS = 216;

static constexpr auto ACTIVE_ANIMATION_TIME_MS = 60,
                      SELECTION_ANIMATION_TIME_MS = 40,
                      INDICATOR_HOVER_ANIMATION_TIME_MS = 50;

static constexpr auto INFO_TOOLTIP_TIME_MS = 1261;

namespace ParamID {
    static constexpr auto 
        globalMix = "Global Mix",
        decay = "Decay",
        crossfadeTime = "Crossfade",
        lowCut = "Low Cut",
        highCut = "High Cut",

        weightingMode = "Weighting Mode",
        strength = "Strength",
        spread = "Spread",

        positionPattern = "Position Pattern",
        positionRate = "Position Rate",
        positionModA = "Position Mod A",
        positionModB = "Position Mod B",

        fieldPattern = "Field Pattern",
        fieldRate = "Field Rate",
        fieldModA = "Field Mod A",
        fieldModB = "Field Mod B";
}

namespace PropertyID {
    static constexpr auto
        selectedIR = "Selected IR",
        selectedControlTab = "Selected Control Tab",

        fileFilter = "IR File FIlter",
        directoryFilter = "IR Directory Filter",
        samplingMode = "IR Sampling Mode",

        tooltipsEnabled = "Tooltips Enabled",
        positionIndicatorStyle = "Position Indicator Style",
        fieldIndicatorStyle = "Field Indicator Style",
        controlRate = "Control Rate";

    namespace Position {
        static constexpr auto
            r = "Position R",
            theta = "Position Theta";
    }

    namespace Point {
        static constexpr auto
            r = "Point R",
            theta = "Point Theta";
    }

    namespace ParameterState {
        static constexpr auto
            pattern = "Pattern Name",
            rate = "Pattern Rate",
            modA = "Pattern Mod A",
            modB = "Pattern Mod B";
    }

    namespace IRSlot {
        static constexpr auto
            filePath = "IR Slot Filepath",
            occupied = "IR Slot Occupied",
            active = "IR Slot Active",
            gainDB = "IR Slot Gain";

        namespace Window {
            static constexpr auto
                start = "IR Slot Window Start",
                length = "IR Slot Window Length";

            namespace Envelope {
                static constexpr auto
                    type = "IR Slot Window Envelope Type",
                    attack = "IR Slot Window Envelope Attack",
                    release = "IR Slot Window Envelope Release";
            }
        }

        namespace AutoSwap {
            static constexpr auto
                swapActive = "IR Slot Swap Active",
                swapMin = "IR Slot Swap Min Time",
                swapMax = "IR Slot Swap Max Time";
        }
    }

    namespace IRDirectory {
        static constexpr auto
            irDirectoryCount = "IR Directory Count";

        inline juce::String irDirectoryActive(int i) { return "IR Directory " + juce::String(i) + " Active"; }
        inline juce::String irDirectoryPath(int i) { return "IR Directory " + juce::String(i) + " Path"; }
    }
}

namespace TreeID {
    static constexpr auto
        guiState = "GUI State",
        patternState = "Pattern State",
        irManagerState = "IR Manager State";

    namespace GUIState {
        static constexpr auto
            fieldCoordinates = "Field Coordinates";
        namespace FieldCoordinates {
            static constexpr auto point = "Point";
        }
    }

    namespace PatternState {
        static constexpr auto
            positionParamStates = "Position Parameter States",
            fieldParamStates = "Field Parameter States";

        inline juce::String paramState(int i) { return "Parameter State " + juce::String(i); }
    }

    namespace IRManager {
        static constexpr auto
            irSlot = "IR Slot";
    }
}

const juce::StringArray controlRates { "20 Hz", "25 Hz", "30 Hz", "40 Hz", "50 Hz", "60 Hz" };
const juce::StringArray weightingModes { "Absolute", "Relative" };
const juce::StringArray positionPatterns { "Vanilla", "Eyes", "Orbit", "Spiral", "Floral", "Lissajous", "Random", "Walk" };
const juce::StringArray fieldPatterns { "Vanilla", "Ring", "Orbits", "Random", "Walk" };

// Aliases
using AudioGraphIOProcessor = juce::AudioProcessorGraph::AudioGraphIOProcessor;
using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

using Bounds = juce::Rectangle<int>;
using BoundsF = juce::Rectangle<float>;

using SpectraData =
    std::array< // Channels
        std::vector< // Partitions
            std::array<float, FFT_SIZE> // Packed FFT output
        >,
    N_CHANNELS>;

// Enums
enum class WeightingMode { WEIGHTING_ABSOLUTE, WEIGHTING_RELATIVE };

enum class Axis { X_AXIS, Y_AXIS }; // Polar coordinate reference axis
enum class PositionPattern { MANUAL, EYES, ORBIT, SPIRAL, LISSAJOUS, FLORAL, RANDOM_DISCRETE, RANDOM_WALK };
enum class FieldPattern { MANUAL, RING, ORBITS, RANDOM_DISCRETE, RANDOM_WALK };

namespace PositionIndicatorStyle {
    static const juce::String
        Mareless = "Mareless",
        Anon = "Anon",
        Aryanne = "Aryanne",
        Fausticorn = "Fausticorn",
        Milky_Way = "Milky Way",
        Nasapone = "Nasapone",
        Sergeant_Reckless = "Sergeant Reckless",
        Tracy_Cage = "Tracy Cage",
        Verity = "Verity";
}
namespace FieldIndicatorStyle {
    static const juce::String
        Mareless = "Mareless",
        Half_Mared = "Half-Mared",
        Mareful = "Mareful";
}

// Structs

struct CartesianCoordinate { 
    float x, y; 
    bool operator==(const CartesianCoordinate& b) const {
        return std::fabs(x - b.x) < EPSILON && std::fabs(y - b.y) < EPSILON;
    }
};

struct PolarCoordinate {
    float r, theta;
    bool operator==(const PolarCoordinate& b) const {
        // Unwrap angle
        float dTheta = std::fmod(std::fabs(theta - b.theta), juce::MathConstants<float>::twoPi);
        if (dTheta > juce::MathConstants<float>::pi) dTheta = juce::MathConstants<float>::twoPi - dTheta;
        return std::fabs(r - b.r) < EPSILON && dTheta < EPSILON;
    }
};