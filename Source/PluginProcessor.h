#pragma once

#include <JuceHeader.h>

class KomprussrAudioProcessor final : public juce::AudioProcessor
{
public:
    KomprussrAudioProcessor();
    ~KomprussrAudioProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return {}; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> gainReductionDb { 0.0f };

private:
    struct ValveModel
    {
        float thresholdOffsetDb, ratio, kneeDb, attackMs, releaseMs, harmonicDrive;
        float evenHarmonic, oddHarmonic, makeupTrimDb;
    };
    static const ValveModel& modelFor (int index) noexcept;

    double currentSampleRate = 44100.0;
    float envelope = 0.0f;
    float smoothedGain = 1.0f;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KomprussrAudioProcessor)
};
