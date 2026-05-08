#pragma once

#include <JuceHeader.h>

class VersionChecker {
private:
	bool isOutdated(const juce::String& currentVersion, const juce::String& latestVersion);

public:
	std::function<void(const juce::String& currentVersion, const juce::String& latestVersion)> outdatedCallback;
	std::function<void(const juce::String& currentVersion)> upToDateCallback;

	void checkForUpdates(const juce::String repo);
};