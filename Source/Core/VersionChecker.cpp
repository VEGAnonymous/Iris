#include "Core/VersionChecker.h"

/* PRIVATE */

bool VersionChecker::isOutdated(const juce::String& currentVersion, const juce::String& latestVersion) {
	juce::StringArray latestTokens, currentTokens;

	latestTokens.addTokens(latestVersion, ".", "");
	currentTokens.addTokens(currentVersion, ".", "");

	const int count = juce::jmax(latestTokens.size(), currentTokens.size());
	for (int i = 0; i < count; ++i) { // Check version number from left to right
		const int latestToken = i < latestTokens.size() ? latestTokens[i].getIntValue() : 0;
		const int currentToken = i < currentTokens.size() ? currentTokens[i].getIntValue() : 0;

		if (latestToken > currentToken) return true;
	}

	return false;
}

/* PUBLIC */

void VersionChecker::checkForUpdates(const juce::String repo) {
	const juce::URL url(
		"https://api.github.com/repos/LYRAnonymous/" + repo + "/releases/latest");

	std::unique_ptr<juce::InputStream> stream(url.createInputStream(
		juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)));

	if (!stream) {
		DBG("VERSION: Failed to fetch releases for " << repo);
		return;
	}

	const juce::String response = stream->readEntireStreamAsString();
	auto json = juce::JSON::parse(response);
	jassert(json.isObject());
	if (!json.isObject()) return;

	const auto* obj = json.getDynamicObject();
	jassert(obj);
	if (!obj) return;

	juce::String latestVersion = obj->getProperty("name").toString().retainCharacters("0123456789.-");
	const juce::String currentVersion = ProjectInfo::versionString;

	DBG("VERSION: Current: " + currentVersion);
	DBG("VERSION: Latest:  " + latestVersion);

	if (isOutdated(currentVersion, latestVersion)) {
		if (outdatedCallback) outdatedCallback(currentVersion, latestVersion);
	}
	else {
		if (upToDateCallback) upToDateCallback(currentVersion);
	}
}
