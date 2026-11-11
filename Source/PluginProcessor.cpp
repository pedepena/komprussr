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
        juce::NormalisableRange<float> (0.0f, 100.0f, 0.01f), 50.0f, "%"));
    layout.add (std::make_unique<juce::AudioParameterFloat> ("make_up_gain", "Make Up Gain",
        juce::NormalisableRange<float> (-12.0f, 24.0f, 0.01f), 0.0f, "dB"));
    juce::StringArray tubes;
    for (auto* name : tubeNames) tubes.add (name);
    layout.add (std::make_unique<juce::AudioParameterChoice> ("tube", "Tube", tubes, 0));
    layout.add (std::make_unique<juce::AudioParameterBool> ("power", "Power", true));
    return layout;
}

void KomprussrAudioProcessor::prepareToPlay (double sampleRate, int)
{
    currentSampleRate = juce::jmax (1.0, sampleRate);
    envelope = 0.0f;
    smoothedGain = 1.0f;
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
        {  0.0f, 1.55f, 8.0f, 32.0f, 520.0f, 0.035f, 0.72f, 0.28f,  0.00f }, // 6386U
        { -1.0f, 1.85f, 6.0f, 20.0f, 360.0f, 0.055f, 0.55f, 0.45f, -0.10f }, // 6BA6
        { -1.5f, 2.05f, 4.5f, 12.0f, 280.0f, 0.080f, 0.62f, 0.38f, -0.15f }, // 6BC8
        {  0.5f, 1.75f, 5.5f, 24.0f, 430.0f, 0.065f, 0.48f, 0.52f, -0.05f }, // 6K7
        { -0.5f, 2.20f, 3.5f,  9.0f, 240.0f, 0.095f, 0.42f, 0.58f, -0.20f }  // 6SK7
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
    const auto compression = parameters.getRawParameterValue ("compression")->load() * 0.01f;
    const auto makeup = dbToGain (parameters.getRawParameterValue ("make_up_gain")->load());
    const auto tube = juce::roundToInt (parameters.getRawParameterValue ("tube")->load());
    const auto& model = modelFor (tube);
    const auto threshold = -30.0f + model.thresholdOffsetDb + (1.0f - compression) * 30.0f;
    const auto attack = std::exp (-1.0f / (float (currentSampleRate) * model.attackMs * 0.001f));
    const auto release = std::exp (-1.0f / (float (currentSampleRate) * model.releaseMs * 0.001f));
    float peakReduction = 0.0f;

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float detector = 0.0f;
        for (int ch = 0; ch < channels; ++ch) detector = juce::jmax (detector, std::abs (buffer.getSample (ch, i)));
        const auto detectorDb = 20.0f * std::log10 (juce::jmax (detector, 1.0e-8f));
        const auto coeff = detector > envelope ? attack : release;
        envelope = coeff * envelope + (1.0f - coeff) * detector;
        const auto levelDb = 20.0f * std::log10 (juce::jmax (envelope, 1.0e-8f));
        const auto over = levelDb - threshold;
        float reductionDb = 0.0f;
        if (model.kneeDb > 0.0f && over > -model.kneeDb * 0.5f && over < model.kneeDb * 0.5f)
        {
            const auto x = over + model.kneeDb * 0.5f;
            reductionDb = (1.0f - 1.0f / model.ratio) * x * x / (2.0f * model.kneeDb);
        }
        else if (over >= model.kneeDb * 0.5f)
            reductionDb = over * (1.0f - 1.0f / model.ratio);

        const auto targetGain = dbToGain (-juce::jmax (0.0f, reductionDb));
        smoothedGain = 0.9995f * smoothedGain + 0.0005f * targetGain;
        peakReduction = juce::jmax (peakReduction, juce::jmax (0.0f, -reductionDb));
        const auto drive = 1.0f + model.harmonicDrive * juce::jlimit (0.0f, 1.0f, -reductionDb / 18.0f);
        for (int ch = 0; ch < channels; ++ch)
        {
            auto x = buffer.getSample (ch, i);
            if (power)
            {
                const auto driven = x * drive;
                // Bounded, level/reduction-dependent asymmetric saturation.
                const auto shaped = std::tanh (driven + model.evenHarmonic * driven * driven)
                                  + model.oddHarmonic * driven * driven * driven * 0.025f;
                x = shaped * smoothedGain * makeup;
            }
            buffer.setSample (ch, i, x);
        }
        juce::ignoreUnused (detectorDb);
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
