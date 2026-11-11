#include "PluginEditor.h"

KomprussrAudioProcessorEditor::KomprussrAudioProcessorEditor (KomprussrAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      tubeAttachment (p.parameters, "tube", tube),
      compressionAttachment (p.parameters, "compression", compression),
      makeupAttachment (p.parameters, "make_up_gain", makeup),
      powerAttachment (p.parameters, "power", power)
{
    setSize (520, 340);
    title.setText ("KOMPRUSSR KU-01", juce::dontSendNotification);
    title.setJustificationType (juce::Justification::centred);
    title.setFont (juce::Font (24.0f, juce::Font::bold));
    addAndMakeVisible (title);
    tube.addItemList ({ "6386U", "6BA6", "6BC8", "6K7", "6SK7" }, 1);
    addAndMakeVisible (tube);
    for (auto* slider : { &compression, &makeup })
    {
        slider->setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
        slider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 22);
        addAndMakeVisible (*slider);
    }
    compression.setName ("Compression");
    compression.setTextValueSuffix (" %");
    makeup.setName ("Make Up Gain");
    makeup.setTextValueSuffix (" dB");
    addAndMakeVisible (power);
    grLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (grLabel);
}

void KomprussrAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff302522));
    g.setColour (juce::Colour (0xffb8392f));
    g.drawRoundedRectangle (getLocalBounds().toFloat().reduced (8.0f), 8.0f, 3.0f);
    g.setColour (juce::Colours::white);
    g.drawText ("TUBE MODEL", 20, 82, 140, 24, juce::Justification::centredLeft);
    g.drawText ("COMPRESSION", 120, 142, 140, 22, juce::Justification::centred);
    g.drawText ("MAKE UP GAIN", 280, 142, 140, 22, juce::Justification::centred);
    grLabel.setText ("GR: " + juce::String (processor.gainReductionDb.load(), 1) + " dB", juce::dontSendNotification);
}

void KomprussrAudioProcessorEditor::resized()
{
    title.setBounds (20, 18, 480, 48);
    tube.setBounds (180, 82, 190, 32);
    compression.setBounds (120, 164, 140, 140);
    makeup.setBounds (280, 164, 140, 140);
    power.setBounds (24, 88, 90, 28);
    grLabel.setBounds (188, 306, 144, 24);
}
