#include "DragAndDropMixin.h"

/* PUBLIC */

bool DragAndDropMixin::isInterestedInFileDrag(const juce::StringArray& filenames) {
    return dragHandler.isInterestedInFileDrag(filenames);
}

void DragAndDropMixin::fileDragEnter(const juce::StringArray& filenames, int x, int y) {
    dragHandler.fileDragEnter(filenames, x, y);
}

void DragAndDropMixin::fileDragExit(const juce::StringArray& filenames) {
    dragHandler.fileDragExit(filenames);
}

void DragAndDropMixin::filesDropped(const juce::StringArray& filenames, int x, int y) {
    dragHandler.filesDropped(filenames, x, y);
}