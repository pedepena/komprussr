#include "UpdateChecker.h"
#include <juce_core/juce_core.h>
#include <juce_data_structures/juce_data_structures.h>
#include <JuceHeader.h>
#include <mutex>

namespace
{
constexpr auto repository = "https://github.com/pedepena/komprussr";
constexpr auto apiUrl = "https://api.github.com/repos/pedepena/komprussr/releases/latest";
constexpr auto requestUserAgent = "KOMPRUSSR-KU-01";
constexpr auto checkIntervalMs = 24LL * 60LL * 60LL * 1000LL;

std::mutex stateMutex;
bool checkRunning = false;
bool updateNoticeUsed = false;
bool sessionQueryStarted = false;

juce::ApplicationProperties& appProperties()
{
    static juce::ApplicationProperties properties;
    static const bool configured = [] (juce::ApplicationProperties& p)
    {
        juce::PropertiesFile::Options options;
        options.applicationName = "KOMPRUSSR";
        options.filenameSuffix = "settings";
        options.folderName = "KOMPRUSSR";
        options.osxLibrarySubFolder = "Application Support";
        options.storageFormat = juce::PropertiesFile::storeAsXML;
        p.setStorageParameters (options);
        return true;
    } (properties);
    juce::ignoreUnused (configured);
    return properties;
}

juce::PropertiesFile* settings()
{
    return appProperties().getUserSettings();
}

bool isNewerVersion (juce::String remote, juce::String local)
{
    if (remote.startsWithIgnoreCase ("v")) remote = remote.substring (1);
    if (local.startsWithIgnoreCase ("v")) local = local.substring (1);
    const auto remoteParts = juce::StringArray::fromTokens (remote, ".", "");
    const auto localParts = juce::StringArray::fromTokens (local, ".", "");
    if (remoteParts.size() != 3 || localParts.size() != 3) return false;
    int r[3] {}, l[3] {};
    for (int i = 0; i < 3; ++i)
    {
        if (remoteParts[i].isEmpty() || localParts[i].isEmpty()
            || remoteParts[i].length() > 9 || localParts[i].length() > 9
            || ! remoteParts[i].containsOnly ("0123456789") || ! localParts[i].containsOnly ("0123456789"))
            return false;
        r[i] = remoteParts[i].getIntValue();
        l[i] = localParts[i].getIntValue();
    }
    for (int i = 0; i < 3; ++i) { if (r[i] > l[i]) return true; if (r[i] < l[i]) return false; }
    return false;
}

class UpdateJob final : public juce::ThreadPoolJob
{
public:
    UpdateJob (juce::Component* component, UpdateChecker::UpdateCallback fn)
        : ThreadPoolJob ("KOMPRUSSR update check"), owner (component), callback (std::move (fn)) {}
    JobStatus runJob() override
    {
        juce::String version, releaseUrl;
        if (UpdateChecker::automaticChecksEnabled())
        if (auto stream = juce::URL (apiUrl).createInputStream (
                juce::URL::InputStreamOptions (juce::URL::ParameterHandling::inAddress)
                    .withConnectionTimeoutMs (5000)
                    .withExtraHeaders ("User-Agent: " + juce::String (requestUserAgent) + "\r\nAccept: application/vnd.github+json\r\n")))
        {
            juce::var json;
            const auto result = juce::JSON::parse (stream->readEntireStreamAsString(), json);
            const auto tag = json.getProperty ("tag_name", {}).toString();
            releaseUrl = json.getProperty ("html_url", {}).toString();
            if (result.wasOk() && isNewerVersion (tag, UpdateChecker::currentVersion()))
                version = tag.startsWithIgnoreCase ("v") ? tag : "v" + tag;
        }

        bool shouldNotify = false;
        {
            const std::lock_guard<std::mutex> lock (stateMutex);
            checkRunning = false;
            auto* file = settings();
            if (file != nullptr)
            {
                file->setValue ("lastUpdateCheckMs", juce::String (juce::Time::currentTimeMillis()));
                file->saveIfNeeded();
            }
            if (version.isNotEmpty() && file != nullptr && file->getBoolValue ("automaticUpdateChecks", true) && ! updateNoticeUsed)
            {
                updateNoticeUsed = true;
                shouldNotify = true;
            }
        }
        if (shouldNotify)
            juce::MessageManager::callAsync ([safe = owner, fn = callback, version, releaseUrl]
            {
                if (safe != nullptr && static_cast<bool> (fn) && releaseUrl.startsWith ("https://github.com/"))
                    fn (version, releaseUrl);
            });
        return jobHasFinished;
    }
private:
    juce::Component::SafePointer<juce::Component> owner;
    UpdateChecker::UpdateCallback callback;
};

juce::ThreadPool& worker()
{
    static juce::ThreadPool pool (1);
    return pool;
}
}

bool UpdateChecker::automaticChecksEnabled()
{
    const std::lock_guard<std::mutex> lock (stateMutex);
    auto* file = settings();
    return file == nullptr || file->getBoolValue ("automaticUpdateChecks", true);
}

void UpdateChecker::setAutomaticChecksEnabled (bool enabled)
{
    const std::lock_guard<std::mutex> lock (stateMutex);
    if (auto* file = settings())
    {
        file->setValue ("automaticUpdateChecks", enabled);
        file->saveIfNeeded();
    }
}

void UpdateChecker::checkIfNeeded (juce::Component* owner, UpdateCallback callback)
{
    if (! automaticChecksEnabled() || owner == nullptr) return;
    const std::lock_guard<std::mutex> lock (stateMutex);
    if (checkRunning || sessionQueryStarted) return;
    auto* file = settings();
    const auto lastCheck = file != nullptr ? file->getValue ("lastUpdateCheckMs", "0").getLargeIntValue() : 0;
    if (lastCheck > 0 && juce::Time::currentTimeMillis() - lastCheck < checkIntervalMs) return;
    checkRunning = true;
    sessionQueryStarted = true;
    worker().addJob (new UpdateJob (owner, std::move (callback)), true);
}

juce::String UpdateChecker::currentVersion() { return JucePlugin_VersionString; }
juce::String UpdateChecker::repositoryUrl() { return repository; }
