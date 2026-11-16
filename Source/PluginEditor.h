#pragma once
#include "PluginProcessor.h"
#include "SettingsPanel.h"

class KomprussrAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                            private juce::Timer
{
public:
    explicit KomprussrAudioProcessorEditor (KomprussrAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    juce::Rectangle<int> scaledBounds (int x, int y, int width, int height) const;
    static float angleForDb (float db) noexcept;

    KomprussrAudioProcessor& processor;
    juce::Slider tube, compression, makeup;
    juce::ToggleButton power { "" };
    juce::AudioProcessorValueTreeState::SliderAttachment tubeAttachment, compressionAttachment, makeupAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;
    std::unique_ptr<juce::Button> settingsButton;
    std::unique_ptr<SettingsPanel> settingsPanel;
    juce::Image panel, scales, tubeGlow, lamp, shadows;
    juce::Image compressionImage, makeupImage, tubeImage;
    juce::Image compressionShadow, makeupShadow, tubeShadow;
    juce::Image compressionLight, makeupLight, tubeLight;
    juce::Image highlights, tumblerOn, tumblerOff, needle, glass, grLabel;
    float lampOpacity = 0.0f, needleDb = 0.0f;
    double lastTickMs = 0.0;
    bool initialisingSize = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KomprussrAudioProcessorEditor)
};
