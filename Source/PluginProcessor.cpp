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
    fastReductionDb = slowReductionDb = 0.0f;
    for (int index = 0; index <= 6000; ++index)
        reductionGainTable[index] = dbToGain (-0.01f * static_cast<float> (index));
    valveOversampling.initProcessing (static_cast<size_t> (juce::jmax (1, samplesPerBlock)));
    valveOversampling.reset();
    oversampledBuffer.setSize (getTotalNumInputChannels(), juce::jmax (1, samplesPerBlock) * 2, false, false, true);
    colourBuffer.setSize (4, juce::jmax (1, samplesPerBlock), false, false, true);
    setLatencySamples (juce::roundToInt (valveOversampling.getLatencyInSamples()));
    activeTube = previousTube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    modelFadeSamples = 0;
    for (auto& channel : dcInput) for (auto& value : channel) value = 0.0f;
    for (auto& channel : dcOutput) for (auto& value : channel) value = 0.0f;
    for (auto& channel : toneState) for (auto& value : channel) value = 0.0f;
    for (auto& channel : transientEnvelope) for (auto& value : channel) value = 0.0f;
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
        {  0.0f, 2.0f, 0.8f, 8.0f, 32.0f, 150.0f, 1500.0f, 0.60f, 0.35f, 0.72f, 0.28f, -0.78f, 9000.0f, 0.40f, 0.00f, 0.00f, 0.000f }, // 6386U
        { -1.0f, 2.5f, 1.0f, 6.0f, 20.0f, 100.0f,  900.0f, 0.50f, 0.50f, 0.55f, 0.45f, -0.15f, 8200.0f, 0.55f, 0.22f, 0.15f, 0.012f }, // 6BA6
        { -1.5f, 3.0f, 1.2f, 4.5f, 12.0f,  70.0f,  600.0f, 0.45f, 0.48f, 0.62f, 0.38f,  1.19f, 7400.0f, 0.60f, 0.42f, 0.35f, 0.020f }, // 6BC8
        {  0.5f, 2.8f, 0.9f, 5.5f, 24.0f, 110.0f, 1000.0f, 0.50f, 0.55f, 0.48f, 0.52f, -0.38f, 6700.0f, 0.60f, 0.32f, 0.10f, -0.015f }, // 6K7
        { -0.5f, 4.0f, 1.5f, 3.5f,  9.0f,  50.0f,  450.0f, 0.35f, 1.70f, 0.42f, 0.58f, -0.41f, 6000.0f, 0.95f, 0.80f, 0.30f, 0.030f }  // 6SK7
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
        fastReductionDb = slowReductionDb = 0.0f;
        gainReductionDb.store (0.0f, std::memory_order_relaxed);
        return;
    }
    const auto compression = parameters.getRawParameterValue ("compression")->load() * 0.01f;
    const auto makeup = dbToGain (parameters.getRawParameterValue ("make_up_gain")->load());
    const auto tube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    const auto& model = modelFor (tube);
    const auto threshold = -6.0f - 34.0f * std::pow (compression, 0.60f) + model.thresholdOffsetDb;
    const auto ratio = 1.25f + (model.maximumRatio - 1.25f) * std::pow (compression, model.curveExponent);
    if (tube != activeTube)
    {
        previousTube = activeTube;
        activeTube = tube;
        modelFadeSamples = modelFadeLength;
    }
    const auto attack = std::exp (-1.0f / (float (currentSampleRate) * model.attackMs * 0.001f));
    const auto fastRelease = std::exp (-1.0f / (float (currentSampleRate) * model.fastReleaseMs * 0.001f));
    const auto slowRelease = std::exp (-1.0f / (float (currentSampleRate) * model.slowReleaseMs * 0.001f));
    const auto detectorAttack = std::exp (-1.0f / (float (currentSampleRate) * 0.00025f));
    const auto detectorRelease = std::exp (-1.0f / (float (currentSampleRate) * 0.002f));
    float peakReduction = 0.0f;

    const auto numSamples = buffer.getNumSamples();
    const auto chunkCapacity = juce::jmax (1, oversampledBuffer.getNumSamples() / 2);
    const auto highRateHz = static_cast<float> (currentSampleRate * 2.0);
    const auto transientAttack = std::exp (-1.0f / (highRateHz * 0.00025f));
    const auto transientRelease = std::exp (-1.0f / (highRateHz * 0.003f));
    float trimGains[5] {};
    float toneCoefficients[5] {};
    for (int index = 0; index < 5; ++index)
    {
        const auto& profile = modelFor (index);
        const auto cutoff = profile.toneCutoffHz;
        toneCoefficients[index] = cutoff / (cutoff + highRateHz / juce::MathConstants<float>::twoPi);
        trimGains[index] = dbToGain (profile.makeupTrimDb);
    }
    for (int chunkStart = 0; chunkStart < numSamples; chunkStart += chunkCapacity)
    {
        const auto chunkSamples = juce::jmin (chunkCapacity, numSamples - chunkStart);
        for (int i = 0; i < chunkSamples; ++i)
        {
            float detectorPower = 0.0f;
            for (int ch = 0; ch < channels; ++ch)
            {
                const auto sample = buffer.getSample (ch, chunkStart + i);
                detectorPower += sample * sample;
            }
            detectorPower /= static_cast<float> (juce::jmax (1, channels));
            const auto envelopeCoeff = detectorPower > envelopeSquared ? detectorAttack : detectorRelease;
            envelopeSquared = envelopeCoeff * envelopeSquared + (1.0f - envelopeCoeff) * detectorPower;
            const auto levelDb = 10.0f * std::log10 (juce::jmax (envelopeSquared, 1.0e-12f));
            const auto over = levelDb - threshold;
            float reductionDb = 0.0f;
            if (compression > 0.0f && model.kneeDb > 0.0f && over > -model.kneeDb * 0.5f && over < model.kneeDb * 0.5f)
            {
                const auto x = over + model.kneeDb * 0.5f;
                reductionDb = (1.0f - 1.0f / ratio) * x * x / (2.0f * model.kneeDb);
            }
            else if (compression > 0.0f && over >= model.kneeDb * 0.5f)
                reductionDb = over * (1.0f - 1.0f / ratio);

            const auto targetReductionDb = juce::jmax (0.0f, reductionDb);
            const auto fastCoeff = targetReductionDb > fastReductionDb ? attack : fastRelease;
            const auto slowCoeff = targetReductionDb > slowReductionDb ? attack : slowRelease;
            fastReductionDb = fastCoeff * fastReductionDb + (1.0f - fastCoeff) * targetReductionDb;
            slowReductionDb = slowCoeff * slowReductionDb + (1.0f - slowCoeff) * targetReductionDb;
            const auto measuredReduction = fastReductionDb * (1.0f - model.slowReleaseMix)
                                         + slowReductionDb * model.slowReleaseMix;
            peakReduction = juce::jmax (peakReduction, measuredReduction);
            const auto levelAmount = juce::jlimit (0.0f, 1.0f, (levelDb + 36.0f) / 30.0f);
            const auto colour = compression > 0.0f
                ? juce::jlimit (0.0f, 0.95f, 0.02f * compression + measuredReduction / 8.0f)
                : 0.0f;
            const auto toneAmount = juce::jlimit (0.0f, 1.0f, measuredReduction / 10.0f);
            const auto trimAmount = juce::jlimit (0.0f, 1.0f, measuredReduction / 6.8f);
            colourBuffer.setSample (0, i, colour);
            colourBuffer.setSample (1, i, levelAmount);
            colourBuffer.setSample (2, i, toneAmount);
            colourBuffer.setSample (3, i, trimAmount);
            const auto lookupPosition = juce::jlimit (0.0f, 6000.0f, measuredReduction * 100.0f);
            const auto lookupIndex = static_cast<int> (lookupPosition);
            const auto lookupFraction = lookupPosition - static_cast<float> (lookupIndex);
            const auto gain = reductionGainTable[lookupIndex]
                + lookupFraction * (reductionGainTable[juce::jmin (6000, lookupIndex + 1)] - reductionGainTable[lookupIndex]);
            for (int ch = 0; ch < channels; ++ch)
                buffer.setSample (ch, chunkStart + i, buffer.getSample (ch, chunkStart + i) * gain * makeup);
        }

        for (int ch = 0; ch < channels; ++ch)
            oversampledBuffer.copyFrom (ch, 0, buffer, ch, chunkStart, chunkSamples);
        auto sourceBlock = juce::dsp::AudioBlock<float> (oversampledBuffer).getSubBlock (0, static_cast<size_t> (chunkSamples));
        auto highRate = valveOversampling.processSamplesUp (sourceBlock);
        const auto highRateChannels = static_cast<int> (highRate.getNumChannels());
        const auto highRateSamples = static_cast<int> (highRate.getNumSamples());
        for (int i = 0; i < highRateSamples; ++i)
        {
            const auto baseIndex = juce::jmin (chunkSamples - 1, i / 2);
            const auto colour = colourBuffer.getSample (0, baseIndex);
            const auto levelAmount = colourBuffer.getSample (1, baseIndex);
            const auto toneAmount = colourBuffer.getSample (2, baseIndex);
            const auto trimAmount = colourBuffer.getSample (3, baseIndex);
            const auto fade = modelFadeSamples > 0 ? 1.0f - float (modelFadeSamples) / float (modelFadeLength) : 1.0f;
            for (int ch = 0; ch < highRateChannels; ++ch)
            {
                const auto input = highRate.getSample (static_cast<size_t> (ch), static_cast<size_t> (i));
                auto processModel = [&] (int modelIndex)
                {
                    const auto& profile = modelFor (modelIndex);
                    auto& transient = transientEnvelope[modelIndex][ch];
                    const auto magnitude = std::abs (input);
                    const auto transientCoeff = magnitude > transient ? transientAttack : transientRelease;
                    const auto transientExcess = juce::jmax (0.0f, magnitude - transient);
                    transient = transientCoeff * transient + (1.0f - transientCoeff) * magnitude;
                    const auto transientDrive = profile.transientGrain * transientExcess;
                    const auto drive = 1.0f + juce::jmin (profile.maximumDrive, profile.harmonicDrive * colour) + transientDrive;
                    const auto driven = input * drive;
                    const auto curvedInput = driven + profile.asymmetry * driven * driven;
                    const auto roundedCurve = std::tanh (curvedInput);
                    const auto angularCurve = curvedInput / (1.0f + 0.75f * std::abs (curvedInput));
                    const auto baseCurve = roundedCurve + profile.curveHardness * (angularCurve - roundedCurve);
                    const auto evenMix = profile.evenHarmonic * (1.0f - 0.65f * levelAmount);
                    const auto oddMix = profile.oddHarmonic * (0.35f + 0.65f * levelAmount);
                    const auto harmonicAmount = profile.harmonicDrive * colour;
                    const auto saturated = baseCurve
                        + evenMix * harmonicAmount * driven * driven
                        + oddMix * harmonicAmount * driven * driven * driven * 0.30f;
                    const auto shaped = input + colour * (saturated - input);
                    auto& x1 = dcInput[modelIndex][ch];
                    auto& y1 = dcOutput[modelIndex][ch];
                    const auto dcBlocked = shaped - x1 + 0.9995f * y1;
                    x1 = shaped;
                    y1 = dcBlocked;
                    auto& tone = toneState[modelIndex][ch];
                    tone += toneCoefficients[modelIndex] * (dcBlocked - tone);
                    const auto trimGain = 1.0f + trimAmount * (trimGains[modelIndex] - 1.0f);
                    return (dcBlocked + toneAmount * (tone - dcBlocked)) * trimGain;
                };
                float output = processModel (activeTube);
                if (modelFadeSamples > 0)
                {
                    const auto oldValue = processModel (previousTube);
                    const auto newValue = output;
                    output = oldValue + fade * (newValue - oldValue);
                }
                highRate.setSample (static_cast<size_t> (ch), static_cast<size_t> (i), output);
            }
            if (modelFadeSamples > 0 && --modelFadeSamples == 0)
                previousTube = activeTube;
        }
        auto outputBlock = juce::dsp::AudioBlock<float> (buffer).getSubBlock (static_cast<size_t> (chunkStart), static_cast<size_t> (chunkSamples));
        valveOversampling.processSamplesDown (outputBlock);
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
