#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

class UpdateChecker final
{
public:
    using UpdateCallback = std::function<void (const juce::String&, const juce::String&)>;
    static bool automaticChecksEnabled();
    static void setAutomaticChecksEnabled (bool enabled);
    static void checkIfNeeded (juce::Component* owner, UpdateCallback callback);
    static juce::String currentVersion();
    static juce::String repositoryUrl();
};
