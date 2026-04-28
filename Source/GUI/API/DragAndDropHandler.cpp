#include "GUI/API/DragAndDropHandler.h"
#include "GUI/Theme/Theme.h"

/* PUBLIC */

bool DragAndDropHandler::isInterestedInFileDrag(const juce::StringArray& filenames) {
	if (!acceptsMultiple && filenames.size() > 1) return false;

	const bool allowFiles = (typeAccepted != DragAndDropType::DIRECTORIES);
	const bool allowDirectories = (typeAccepted != DragAndDropType::FILES);

	auto testFilename = [&](const juce::String& filename) -> bool {
		juce::File file(filename);
		if (file.isDirectory()) return allowDirectories;
		if (file.existsAsFile()) {
			if (!allowFiles) return false;
			if (fileFilter && !fileFilter(file)) return false;
			return true;
		}
		return false;
		};

	if (acceptsMultiple) {
		for (const auto& filename : filenames) if (!testFilename(filename)) return false;
	} else return testFilename(filenames[0]);

	return true;
}

void DragAndDropHandler::fileDragEnter(const juce::StringArray& filenames, int /*x*/, int /*y*/) {
	if (isInterestedInFileDrag(filenames) && onHoverChanged)
		onHoverChanged(true);
}

void DragAndDropHandler::fileDragExit(const juce::StringArray& filenames) {
	if (isInterestedInFileDrag(filenames) && onHoverChanged)
		onHoverChanged(false);
}

void DragAndDropHandler::filesDropped(const juce::StringArray& filenames, int /*x*/, int /*y*/) {
	auto handleDrop = [&](const juce::String filename) {
		const juce::File file(filename);
		if (file.isDirectory() && onDirectoryDropped) onDirectoryDropped(file);
		else if (file.existsAsFile() && onFileDropped) onFileDropped(file);
	};

	if (acceptsMultiple) {
		for (const juce::String& filename : filenames) handleDrop(filename);
	} else handleDrop(filenames[0]);

	if (onHoverChanged) onHoverChanged(false);
}