#pragma once

#include "DragAndDropHandler.h"

#include <JuceHeader.h>

class DragAndDropMixin : public juce::FileDragAndDropTarget {
public:
	DragAndDropHandler dragHandler;

	bool isInterestedInFileDrag(const juce::StringArray& filenames) override;
	void fileDragEnter(const juce::StringArray& filenames, int x, int y) override;
	void fileDragExit(const juce::StringArray& filenames) override;
	void filesDropped(const juce::StringArray& filenames, int x, int y) override;
};