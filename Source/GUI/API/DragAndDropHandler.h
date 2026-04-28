#pragma once

#include <JuceHeader.h>

class DragAndDropHandler : public juce::FileDragAndDropTarget {
public:
	enum class DragAndDropType { FILES, DIRECTORIES, FILES_AND_DIRECTORIES };

	DragAndDropType typeAccepted = DragAndDropType::FILES_AND_DIRECTORIES;
	bool acceptsMultiple = false;

	std::function<void(const juce::File&)> onFileDropped;
	std::function<void(const juce::File&)> onDirectoryDropped;
	std::function<bool(const juce::File&)> fileFilter;

	std::function<void(bool)> onHoverChanged;

	bool isInterestedInFileDrag(const juce::StringArray& filenames) override;
	void fileDragEnter(const juce::StringArray& filenames, int x, int y) override;
	void fileDragExit(const juce::StringArray& filenames) override;
	void filesDropped(const juce::StringArray& filenames, int x, int y) override;
};