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
    colourBuffer.setSize (5, juce::jmax (1, samplesPerBlock), false, false, true);
    setLatencySamples (juce::roundToInt (valveOversampling.getLatencyInSamples()));
    activeTube = previousTube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    modelFadeSamples = 0;
    for (auto& channel : dcInput) for (auto& value : channel) value = 0.0f;
    for (auto& channel : dcOutput) for (auto& value : channel) value = 0.0f;
    for (auto& channel : toneState) for (auto& value : channel) value = 0.0f;
    for (auto& channel : splitLowState) for (auto& value : channel) value = 0.0f;
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
        {  0.0f, 2.0f, 0.8f, 8.0f, 32.0f, 150.0f, 1500.0f, 0.60f, 0.35f, 0.72f, 0.28f, -0.78f, 9000.0f, 0.40f, 0.00f, 0.00f, 0.000f, 2.5f, 0.0f, 0 }, // 6386U
        { -1.0f, 2.5f, 1.0f, 6.0f, 20.0f, 100.0f,  900.0f, 0.50f, 0.50f, 0.55f, 0.45f, -0.15f, 8200.0f, 0.55f, 0.15f, 0.15f, 0.120f, 3.0f, 800.0f, 1 }, // 6BA6
        { -1.5f, 3.0f, 1.2f, 4.5f, 12.0f,  70.0f,  600.0f, 0.45f, 0.48f, 0.62f, 0.38f,  1.19f, 7400.0f, 0.60f, 0.30f, 0.35f, 0.200f, 3.0f, 400.0f, 1 }, // 6BC8
        {  0.5f, 2.8f, 0.9f, 5.5f, 24.0f, 110.0f, 1000.0f, 0.50f, 0.55f, 0.48f, 0.60f, -0.38f, 6700.0f, 0.60f, 0.35f, 0.10f, -0.050f, 3.0f, 300.0f, 2 }, // 6K7
        { -0.5f, 4.0f, 1.5f, 3.5f,  9.0f,  50.0f,  450.0f, 0.35f, 1.45f, 0.42f, 0.58f,  0.00f, 6000.0f, 0.85f, 0.60f, 0.30f, 0.040f, 2.0f, 800.0f, 2 }  // 6SK7
    };
    return models[juce::jlimit (0, 4, index)];
}

void KomprussrAudioProcessor::processBlockImpl (juce::AudioBuffer<double>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const auto channels = getTotalNumInputChannels();
    for (int ch = getTotalNumOutputChannels(); ch < buffer.getNumChannels(); ++ch)
        buffer.clear (ch, 0, buffer.getNumSamples());

    const auto power = parameters.getRawParameterValue ("power")->load() >= 0.5f;
    if (! power)
    {
        envelopeSquared = 0.0;
        fastReductionDb = slowReductionDb = 0.0;
        gainReductionDb.store (0.0f, std::memory_order_relaxed);
        return;
    }
    const double compression = parameters.getRawParameterValue ("compression")->load() * 0.01;
    const double makeup = dbToGain (parameters.getRawParameterValue ("make_up_gain")->load());
    const auto tube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    const auto& model = modelFor (tube);
    const double threshold = -6.0 - 34.0 * std::pow (compression, 0.60) + model.thresholdOffsetDb;
    const double ratio = 1.25 + (model.maximumRatio - 1.25) * std::pow (compression, (double) model.curveExponent);
    if (tube != activeTube)
    {
        previousTube = activeTube;
        activeTube = tube;
        modelFadeSamples = modelFadeLength;
    }
    const double sr = currentSampleRate;
    const double attack = std::exp (-1.0 / (sr * model.attackMs * 0.001));
    const double fastRelease = std::exp (-1.0 / (sr * model.fastReleaseMs * 0.001));
    const double slowRelease = std::exp (-1.0 / (sr * model.slowReleaseMs * 0.001));
    const double detectorAttack = std::exp (-1.0 / (sr * 0.00025));
    const double detectorRelease = std::exp (-1.0 / (sr * 0.002));
    float peakReduction = 0.0f;

    const auto numSamples = buffer.getNumSamples();
    const auto chunkCapacity = juce::jmax (1, oversampledBuffer.getNumSamples() / 2);
    const double highRateHz = sr * 2.0;
    const double transientAttack = std::exp (-1.0 / (highRateHz * 0.00025));
    const double transientRelease = std::exp (-1.0 / (highRateHz * 0.003));
    double trimGains[5] {};
    double toneCoefficients[5] {};
    double splitCoefficients[5] {};
    for (int index = 0; index < 5; ++index)
    {
        const auto& profile = modelFor (index);
        const double cutoff = profile.toneCutoffHz;
        toneCoefficients[index] = cutoff / (cutoff + highRateHz / juce::MathConstants<double>::twoPi);
        const double splitHz = profile.splitHz;
        splitCoefficients[index] = splitHz > 0.0 ? splitHz / (splitHz + highRateHz / juce::MathConstants<double>::twoPi) : 0.0;
        trimGains[index] = dbToGain (profile.makeupTrimDb);
    }
    for (int chunkStart = 0; chunkStart < numSamples; chunkStart += chunkCapacity)
    {
        const auto chunkSamples = juce::jmin (chunkCapacity, numSamples - chunkStart);
        for (int i = 0; i < chunkSamples; ++i)
        {
            double detectorPower = 0.0;
            for (int ch = 0; ch < channels; ++ch)
            {
                const auto sample = buffer.getSample (ch, chunkStart + i);
                detectorPower += sample * sample;
            }
            detectorPower /= static_cast<double> (juce::jmax (1, channels));
            const auto envelopeCoeff = detectorPower > envelopeSquared ? detectorAttack : detectorRelease;
            envelopeSquared = envelopeCoeff * envelopeSquared + (1.0 - envelopeCoeff) * detectorPower;
            const double levelDb = 10.0 * std::log10 (juce::jmax (envelopeSquared, 1.0e-12));
            const double over = levelDb - threshold;
            double reductionDb = 0.0;
            if (compression > 0.0 && model.kneeDb > 0.0f && over > -model.kneeDb * 0.5 && over < model.kneeDb * 0.5)
            {
                const double x = over + model.kneeDb * 0.5;
                reductionDb = (1.0 - 1.0 / ratio) * x * x / (2.0 * model.kneeDb);
            }
            else if (compression > 0.0 && over >= model.kneeDb * 0.5)
                reductionDb = over * (1.0 - 1.0 / ratio);

            const double targetReductionDb = juce::jmax (0.0, reductionDb);
            const auto fastCoeff = targetReductionDb > fastReductionDb ? attack : fastRelease;
            const auto slowCoeff = targetReductionDb > slowReductionDb ? attack : slowRelease;
            fastReductionDb = fastCoeff * fastReductionDb + (1.0 - fastCoeff) * targetReductionDb;
            slowReductionDb = slowCoeff * slowReductionDb + (1.0 - slowCoeff) * targetReductionDb;
            const float measuredReduction = fastReductionDb * (1.0 - model.slowReleaseMix)
                                           + slowReductionDb * model.slowReleaseMix;
            peakReduction = juce::jmax (peakReduction, measuredReduction);
            const double levelAmount = juce::jlimit (0.0, 1.0, (levelDb + 36.0) / 30.0);
            const double colour = compression > 0.0
                ? juce::jlimit (0.0, 0.95, 0.02 * compression + measuredReduction / 8.0)
                : 0.0;
            const double toneAmount = juce::jlimit (0.0, 1.0, measuredReduction / 10.0);
            const double trimAmount = juce::jlimit (0.0, 1.0, measuredReduction / 6.8);
            colourBuffer.setSample (0, i, colour);
            colourBuffer.setSample (1, i, levelAmount);
            colourBuffer.setSample (2, i, toneAmount);
            colourBuffer.setSample (3, i, trimAmount);
            colourBuffer.setSample (4, i, measuredReduction);
            const double lookupPosition = juce::jlimit (0.0, 6000.0, measuredReduction * 100.0);
            const auto lookupIndex = static_cast<int> (lookupPosition);
            const double lookupFraction = lookupPosition - static_cast<double> (lookupIndex);
            const double gain = reductionGainTable[lookupIndex]
                + lookupFraction * (reductionGainTable[juce::jmin (6000, lookupIndex + 1)] - reductionGainTable[lookupIndex]);
            for (int ch = 0; ch < channels; ++ch)
                buffer.setSample (ch, chunkStart + i, buffer.getSample (ch, chunkStart + i) * gain * makeup);
        }

        for (int ch = 0; ch < channels; ++ch)
            oversampledBuffer.copyFrom (ch, 0, buffer, ch, chunkStart, chunkSamples);
        auto sourceBlock = juce::dsp::AudioBlock<double> (oversampledBuffer).getSubBlock (0, static_cast<size_t> (chunkSamples));
        auto highRate = valveOversampling.processSamplesUp (sourceBlock);
        const auto highRateChannels = static_cast<int> (highRate.getNumChannels());
        const auto highRateSamples = static_cast<int> (highRate.getNumSamples());
        for (int i = 0; i < highRateSamples; ++i)
        {
            const auto baseIndex = juce::jmin (chunkSamples - 1, i / 2);
            const double colour = colourBuffer.getSample (0, baseIndex);
            const double levelAmount = colourBuffer.getSample (1, baseIndex);
            const double toneAmount = colourBuffer.getSample (2, baseIndex);
            const double trimAmount = colourBuffer.getSample (3, baseIndex);
            const float measuredReduction = colourBuffer.getSample (4, baseIndex);
            const double fade = modelFadeSamples > 0 ? 1.0 - double (modelFadeSamples) / double (modelFadeLength) : 1.0;
            for (int ch = 0; ch < highRateChannels; ++ch)
            {
                const double input = highRate.getSample (static_cast<size_t> (ch), static_cast<size_t> (i));
                auto processModel = [&] (int modelIndex) -> double
                {
                    if (compression <= 0.0)
                        return input;
                    const auto& profile = modelFor (modelIndex);
                    auto& transient = transientEnvelope[modelIndex][ch];
                    const double magnitude = std::abs (input);
                    const auto transientCoeff = magnitude > transient ? transientAttack : transientRelease;
                    const double transientExcess = juce::jmax (0.0, magnitude - transient);
                    transient = transientCoeff * transient + (1.0 - transientCoeff) * magnitude;
                    const double transientDrive = profile.transientGrain * transientExcess;
                    const double drive = 1.0 + juce::jmin ((double) profile.maximumDrive, profile.harmonicDrive * colour) + transientDrive;
                    auto& splitLow = splitLowState[modelIndex][ch];
                    splitLow += splitCoefficients[modelIndex] * (input - splitLow);
                    const double low = splitLow;
                    const double high = input - low;
                    const double selectedBand = profile.splitMode == 1 ? low : profile.splitMode == 2 ? high : input;
                    const double preGain = profile.referenceGain * dbToGain (0.3 * measuredReduction);
                    const double driven = selectedBand * preGain * drive;
                    const double curvedInput = driven + profile.asymmetry * driven * driven;
                    const double roundedCurve = std::tanh (curvedInput);
                    const double angularCurve = curvedInput / (1.0 + 0.75 * std::abs (curvedInput));
                    const double baseCurve = roundedCurve + profile.curveHardness * (angularCurve - roundedCurve);
                    const double evenMix = profile.evenHarmonic * (1.0 - 0.65 * levelAmount);
                    const double oddMix = profile.oddHarmonic * (0.35 + 0.65 * levelAmount);
                    const double harmonicAmount = profile.harmonicDrive * colour;
                    const double saturated = baseCurve
                        + evenMix * harmonicAmount * driven * driven
                        + oddMix * harmonicAmount * driven * driven * driven * 0.30;
                    const double normalizedSaturation = saturated / preGain;
                    const double shaped = profile.splitMode == 1 ? high + selectedBand + colour * (normalizedSaturation - selectedBand)
                                        : profile.splitMode == 2 ? low + selectedBand + colour * (normalizedSaturation - selectedBand)
                                                                 : input + colour * (normalizedSaturation - input);
                    auto& x1 = dcInput[modelIndex][ch];
                    auto& y1 = dcOutput[modelIndex][ch];
                    const double dcBlocked = shaped - x1 + 0.9995 * y1;
                    x1 = shaped;
                    y1 = dcBlocked;
                    auto& tone = toneState[modelIndex][ch];
                    tone += toneCoefficients[modelIndex] * (dcBlocked - tone);
                    const double trimGain = 1.0 + trimAmount * (trimGains[modelIndex] - 1.0);
                    return (dcBlocked + toneAmount * (tone - dcBlocked)) * trimGain;
                };
                double output = processModel (activeTube);
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
        auto outputBlock = juce::dsp::AudioBlock<double> (buffer).getSubBlock (static_cast<size_t> (chunkStart), static_cast<size_t> (chunkSamples));
        valveOversampling.processSamplesDown (outputBlock);
    }
    gainReductionDb.store (peakReduction, std::memory_order_relaxed);
}

void KomprussrAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto numChannels = buffer.getNumChannels();
    const auto numSamples  = buffer.getNumSamples();

    // No reallocation as long as prepareToPlay reserved enough (see below)
    doubleBuffer.setSize (numChannels, numSamples, false, false, true);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* src = buffer.getReadPointer (ch);
        auto* dst = doubleBuffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            dst[i] = static_cast<double> (src[i]);
    }

    processBlockImpl (doubleBuffer, midi);

    for (int ch = 0; ch < numChannels; ++ch)
    {
        const auto* src = doubleBuffer.getReadPointer (ch);
        auto* dst = buffer.getWritePointer (ch);
        for (int i = 0; i < numSamples; ++i)
            dst[i] = static_cast<float> (src[i]);
    }
}

void KomprussrAudioProcessor::processBlock (juce::AudioBuffer<double>& buffer, juce::MidiBuffer& midi)
{
    processBlockImpl(buffer, midi);
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
