#pragma once
#include "PluginProcessor.h"

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
    juce::Image panel, scales, tubeGlow, lamp, shadows, compressionImage, makeupImage, tubeImage;
    juce::Image highlights, tumblerOn, tumblerOff, needle, glass, grLabel;
    float lampOpacity = 0.0f, needleDb = 0.0f;
    double lastTickMs = 0.0;
    bool initialisingSize = true;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KomprussrAudioProcessorEditor)
};
