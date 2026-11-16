#pragma once
#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>
#include <memory>

class SettingsPanel final : public juce::Component
{
public:
    explicit SettingsPanel (std::function<void()> closeRequested);
    ~SettingsPanel() override;
    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseDown (const juce::MouseEvent&) override;
    void showUpdateNotice (const juce::String& version, const juce::String& url);
private:
    class ManualContent;
    class AboutContent;
    void selectTab (int tab);
    void close();
    void openLink (const juce::String& url);
    std::function<void()> closeRequested;
    juce::TextButton aboutTab { "About" }, manualTab { "Manual" }, closeButton { "X" };
    juce::Viewport viewport;
    std::unique_ptr<AboutContent> aboutContent;
    std::unique_ptr<ManualContent> manualContent;
    juce::Label updateMessage;
    juce::TextButton downloadButton { "Download" }, updateCloseButton { "Close" };
    int activeTab = 0;
    bool showingUpdate = false;
    juce::String pendingUrl;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SettingsPanel)
};
