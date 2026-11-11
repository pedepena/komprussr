#pragma once
#include "PluginProcessor.h"

class KomprussrAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit KomprussrAudioProcessorEditor (KomprussrAudioProcessor&);
    void paint (juce::Graphics&) override;
    void resized() override;
private:
    KomprussrAudioProcessor& processor;
    juce::ComboBox tube;
    juce::Slider compression, makeup;
    juce::ToggleButton power { "ON" };
    juce::Label title, grLabel;
    juce::AudioProcessorValueTreeState::ComboBoxAttachment tubeAttachment;
    juce::AudioProcessorValueTreeState::SliderAttachment compressionAttachment, makeupAttachment;
    juce::AudioProcessorValueTreeState::ButtonAttachment powerAttachment;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KomprussrAudioProcessorEditor)
};
