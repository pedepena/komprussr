#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
constexpr const char* tubeNames[] = { "6386U", "6BA6", "6BC8", "6K7", "6SK7" };
float dbToGain (float db) noexcept { return std::pow (10.0f, db / 20.0f); }
}

KomprussrAudioProcessor::KomprussrAudioProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
{}

juce::AudioProcessorValueTreeState::ParameterLayout KomprussrAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;
    layout.add (std::make_unique<juce::AudioParameterFloat> ("compression", "Compression",
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, "%"));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("make_up_gain", "Make Up Gain",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.01f), 0.0f, "dB"));
    juce::StringArray tubes;
    for (auto* name : tubeNames) tubes.add (name);
    layout.add (std::make_unique<juce::AudioParameterChoice> ("tube", "Tube", tubes, tubes.indexOf ("6386U")));
    layout.add (std::make_unique<juce::AudioParameterBool> ("power", "Power", true));
    return layout;
}

void KomprussrAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    currentSampleRate = juce::jmax (1.0, sampleRate);
    envelopeSquared = 0.0f;
    smoothedGain = 1.0f;
    valveOversampling.initProcessing (static_cast<size_t> (juce::jmax (1, samplesPerBlock)));
    valveOversampling.reset();
    oversampledBuffer.setSize (getTotalNumInputChannels(), juce::jmax (1, samplesPerBlock) * 2, false, false, true);
    setLatencySamples (juce::roundToInt (valveOversampling.getLatencyInSamples()));
    activeTube = previousTube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    modelFadeSamples = 0;
    for (auto& channel : dcInput) for (auto& value : channel) value = 0.0f;
    for (auto& channel : dcOutput) for (auto& value : channel) value = 0.0f;
    for (auto& channel : toneState) for (auto& value : channel) value = 0.0f;
}
void KomprussrAudioProcessor::releaseResources() {}

bool KomprussrAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& in = layouts.getMainInputChannelSet();
    const auto& out = layouts.getMainOutputChannelSet();
    return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
}

const KomprussrAudioProcessor::ValveModel& KomprussrAudioProcessor::modelFor (int index) noexcept
{
    // Perceptual starting profiles, not fitted tube-circuit models.
    static constexpr ValveModel models[] = {
        {  0.0f, 1.55f, 8.0f, 32.0f, 520.0f, 0.035f, 0.72f, 0.28f,  0.00f, 9000.0f, 0.055f }, // 6386U
        { -1.0f, 1.85f, 6.0f, 20.0f, 360.0f, 0.055f, 0.55f, 0.45f, -0.10f, 8200.0f, 0.075f }, // 6BA6
        { -1.5f, 2.05f, 4.5f, 12.0f, 280.0f, 0.080f, 0.62f, 0.38f, -0.15f, 7400.0f, 0.095f }, // 6BC8
        {  0.5f, 1.75f, 5.5f, 24.0f, 430.0f, 0.065f, 0.48f, 0.52f, -0.05f, 6700.0f, 0.080f }, // 6K7
        { -0.5f, 2.20f, 3.5f,  9.0f, 240.0f, 0.095f, 0.42f, 0.58f, -0.20f, 6000.0f, 0.110f }  // 6SK7
    };
    return models[juce::jlimit (0, 4, index)];
}

void KomprussrAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto channels = getTotalNumInputChannels();
    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const auto power = parameters.getRawParameterValue ("power")->load() >= 0.5f;
    if (! power)
    {
        envelopeSquared = 0.0f;
        smoothedGain = 1.0f;
        gainReductionDb.store (0.0f, std::memory_order_relaxed);
        return;
    }
    const auto compression = parameters.getRawParameterValue ("compression")->load() * 0.01f;
    const auto makeup = dbToGain (parameters.getRawParameterValue ("make_up_gain")->load());
    const auto tube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    const auto& model = modelFor (tube);
    const auto threshold = -30.0f + model.thresholdOffsetDb + (1.0f - compression) * 30.0f;
    if (tube != activeTube)
    {
        previousTube = activeTube;
        activeTube = tube;
        modelFadeSamples = modelFadeLength;
    }
    const auto attack = std::exp (-1.0f / (float (currentSampleRate) * model.attackMs * 0.001f));
    float peakReduction = 0.0f;

    const auto numSamples = buffer.getNumSamples();
    const auto copySamples = juce::jmin (numSamples, oversampledBuffer.getNumSamples() / 2);
    for (int ch = 0; ch < channels; ++ch)
        oversampledBuffer.copyFrom (ch, 0, buffer, ch, 0, copySamples);
    auto sourceBlock = juce::dsp::AudioBlock<float> (oversampledBuffer).getSubBlock (0, static_cast<size_t> (copySamples));
    auto highRate = valveOversampling.processSamplesUp (sourceBlock);
    const auto highRateChannels = static_cast<int> (highRate.getNumChannels());
    const auto highRateSamples = static_cast<int> (highRate.getNumSamples());
    const auto highRateHz = static_cast<float> (currentSampleRate * 2.0);
    float toneCoefficients[5] {};
    for (int index = 0; index < 5; ++index)
        toneCoefficients[index] = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * modelFor (index).toneCutoffHz / highRateHz);
    for (int i = 0; i < highRateSamples; ++i)
    {
        const auto fade = modelFadeSamples > 0 ? 1.0f - float (modelFadeSamples) / float (modelFadeLength) : 1.0f;
        for (int ch = 0; ch < highRateChannels; ++ch)
        {
            const auto input = highRate.getSample (static_cast<size_t> (ch), static_cast<size_t> (i));
            auto processModel = [&] (int modelIndex)
            {
                const auto& profile = modelFor (modelIndex);
                const auto colour = 0.05f + 0.95f * compression;
                const auto drive = 1.0f + juce::jmin (profile.harmonicDrive, profile.maximumDrive) * colour;
                const auto driven = input * drive;
                const auto shaped = std::tanh (driven + profile.evenHarmonic * driven * driven * 0.30f * colour)
                                  + profile.oddHarmonic * driven * driven * driven * 0.30f * colour;
                auto& x1 = dcInput[modelIndex][ch];
                auto& y1 = dcOutput[modelIndex][ch];
                const auto dcBlocked = shaped - x1 + 0.9995f * y1;
                x1 = shaped;
                y1 = dcBlocked;
                auto& tone = toneState[modelIndex][ch];
                tone += toneCoefficients[modelIndex] * (dcBlocked - tone);
                return tone * dbToGain (profile.makeupTrimDb);
            };
            float output = 0.0f;
            if (modelFadeSamples > 0)
            {
                const auto oldValue = processModel (previousTube);
                const auto newValue = processModel (activeTube);
                output = oldValue + fade * (newValue - oldValue);
            }
            else
                output = processModel (activeTube);
            highRate.setSample (static_cast<size_t> (ch), static_cast<size_t> (i), output);
        }
        if (modelFadeSamples > 0 && --modelFadeSamples == 0)
            previousTube = activeTube;
    }
    auto outputBlock = juce::dsp::AudioBlock<float> (buffer).getSubBlock (0, static_cast<size_t> (copySamples));
    valveOversampling.processSamplesDown (outputBlock);

    for (int i = 0; i < numSamples; ++i)
    {
        float detectorPower = 0.0f;
        for (int ch = 0; ch < channels; ++ch)
        {
            const auto sample = buffer.getSample (ch, i);
            detectorPower += sample * sample;
        }
        detectorPower /= static_cast<float> (juce::jmax (1, channels));
        const auto detectorCoeff = detectorPower > envelopeSquared
            ? attack : std::exp (-1.0f / (float (currentSampleRate) * model.releaseMs * 0.001f));
        envelopeSquared = detectorCoeff * envelopeSquared + (1.0f - detectorCoeff) * detectorPower;
        const auto levelDb = 10.0f * std::log10 (juce::jmax (envelopeSquared, 1.0e-12f));
        const auto over = levelDb - threshold;
        float reductionDb = 0.0f;
        if (compression > 0.0f && model.kneeDb > 0.0f && over > -model.kneeDb * 0.5f && over < model.kneeDb * 0.5f)
        {
            const auto x = over + model.kneeDb * 0.5f;
            reductionDb = (1.0f - 1.0f / model.ratio) * x * x / (2.0f * model.kneeDb);
        }
        else if (compression > 0.0f && over >= model.kneeDb * 0.5f)
            reductionDb = over * (1.0f - 1.0f / model.ratio);

        const auto targetGain = dbToGain (-juce::jmax (0.0f, reductionDb));
        const auto programAmount = juce::jlimit (0.0f, 1.0f, (levelDb - threshold + 12.0f) / 24.0f);
        const auto releaseMs = model.releaseMs * (0.60f + 0.80f * programAmount);
        const auto gainCoeff = targetGain < smoothedGain ? attack
            : std::exp (-1.0f / (float (currentSampleRate) * releaseMs * 0.001f));
        smoothedGain = gainCoeff * smoothedGain + (1.0f - gainCoeff) * targetGain;
        const auto measuredReduction = juce::jmax (0.0f, -20.0f * std::log10 (juce::jmax (smoothedGain, 1.0e-8f)));
        peakReduction = juce::jmax (peakReduction, measuredReduction);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto x = buffer.getSample (ch, i);
            x *= smoothedGain * makeup;
            buffer.setSample (ch, i, x);
        }
    }
    gainReductionDb.store (peakReduction, std::memory_order_relaxed);
}

void KomprussrAudioProcessor::getStateInformation (juce::MemoryBlock& dest)
{
    juce::MemoryOutputStream stream (dest, false);
    parameters.state.writeToStream (stream);
}
void KomprussrAudioProcessor::setStateInformation (const void* data, int size)
{
    if (auto tree = juce::ValueTree::readFromData (data, (size_t) size); tree.isValid())
        parameters.replaceState (tree);
}

juce::AudioProcessorEditor* KomprussrAudioProcessor::createEditor() { return new KomprussrAudioProcessorEditor (*this); }
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new KomprussrAudioProcessor(); }
