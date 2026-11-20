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
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}
    void getStateInformation (juce::MemoryBlock&) override;
    void setStateInformation (const void*, int) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
    juce::AudioProcessorValueTreeState parameters;
    std::atomic<float> gainReductionDb { 0.0f };

private:
    struct ValveModel
    {
        float thresholdOffsetDb, maximumRatio, curveExponent, kneeDb, attackMs;
        float fastReleaseMs, slowReleaseMs, slowReleaseMix, harmonicDrive;
        float evenHarmonic, oddHarmonic, makeupTrimDb, toneCutoffHz, maximumDrive;
        float curveHardness, transientGrain, asymmetry;
        float referenceGain, splitHz;
        int splitMode;
    };
    static const ValveModel& modelFor (int index) noexcept;

    double currentSampleRate = 44100.0;
    float envelopeSquared = 0.0f;
    float fastReductionDb = 0.0f, slowReductionDb = 0.0f;
    juce::dsp::Oversampling<float> valveOversampling { 2, 1, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR };
    juce::AudioBuffer<float> oversampledBuffer;
    juce::AudioBuffer<float> colourBuffer;
    int activeTube = 0, previousTube = 0;
    int modelFadeSamples = 0;
    static constexpr int modelFadeLength = 512;
    float reductionGainTable[6001] {};
    float dcInput[5][2] {}, dcOutput[5][2] {}, toneState[5][2] {}, splitLowState[5][2] {}, transientEnvelope[5][2] {};
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (KomprussrAudioProcessor)
};
