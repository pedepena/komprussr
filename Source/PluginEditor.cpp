#include "PluginEditor.h"
#include <BinaryData.h>

namespace
{
juce::Image resourceImage (const char* name)
{
    int size = 0;
    if (auto* data = BinaryData::getNamedResource (name, size))
        return juce::ImageFileFormat::loadFrom (data, static_cast<size_t> (size));
    jassertfalse;
    return {};
}

void drawLayer (juce::Graphics& g, const juce::Image& image, float x, float y, float w, float h)
{
    if (image.isValid())
        g.drawImage (image, juce::roundToInt (x), juce::roundToInt (y), juce::roundToInt (w),
                     juce::roundToInt (h), 0, 0, image.getWidth(), image.getHeight());
}

void drawRotatedLayer (juce::Graphics& g, const juce::Image& image, float x, float y, float w, float h,
                       float pivotX, float pivotY, float degrees)
{
    if (! image.isValid()) return;
    g.saveState();
    g.addTransform (juce::AffineTransform::rotation (juce::degreesToRadians (degrees), pivotX, pivotY));
    drawLayer (g, image, x, y, w, h);
    g.restoreState();
}

juce::Image makeValveGlowLayer (const juce::Image& source)
{
    if (! source.isValid()) return {};
    auto glow = source.createCopy();
    juce::Image::BitmapData pixels (glow, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < pixels.height; ++y)
        for (int x = 0; x < pixels.width; ++x)
        {
            const auto colour = pixels.getPixelColour (x, y);
            const auto red = static_cast<float> (colour.getRed()) / 255.0f;
            const auto green = static_cast<float> (colour.getGreen()) / 255.0f;
            const auto blue = static_cast<float> (colour.getBlue()) / 255.0f;
            const auto warmth = juce::jlimit (0.0f, 1.0f, (red - juce::jmax (green, blue) - 0.08f) / 0.28f);
            const auto brightness = juce::jlimit (0.0f, 1.0f, (red - 0.28f) / 0.38f);
            const auto greenLight = juce::jlimit (0.0f, 1.0f, (green - 0.06f) / 0.24f);
            const auto alpha = static_cast<float> (colour.getAlpha()) * warmth * brightness * greenLight;
            pixels.setPixelColour (x, y, colour.withAlpha (static_cast<juce::uint8> (juce::roundToInt (alpha))));
        }
    return glow;
}

void drawRotatedHighlight (juce::Graphics& g, const juce::Image& image, int x, int y, int width, int height,
                           float pivotX, float pivotY, float degrees)
{
    g.saveState();
    g.reduceClipRegion (juce::Rectangle<int> (x, y, width, height));
    drawRotatedLayer (g, image, 0, 0, 1376, 768, pivotX, pivotY, degrees);
    g.restoreState();
}
}

KomprussrAudioProcessorEditor::KomprussrAudioProcessorEditor (KomprussrAudioProcessor& p)
    : AudioProcessorEditor (&p), processor (p),
      tubeAttachment (p.parameters, "tube", tube),
      compressionAttachment (p.parameters, "compression", compression),
      makeupAttachment (p.parameters, "make_up_gain", makeup),
      powerAttachment (p.parameters, "power", power),
      panel (resourceImage ("panel_base_off_png")), scales (resourceImage ("scales_labels_png")),
      tubeGlow (makeValveGlowLayer (resourceImage ("tubes_on_png"))), lamp (resourceImage ("lamp_on_png")),
      shadows (resourceImage ("knob_shadows_png")),
      compressionImage (resourceImage ("knob_compression2x_png")),
      makeupImage (resourceImage ("knob_makeup2x_png")), tubeImage (resourceImage ("knob_tube2x_png")),
      highlights (resourceImage ("knob_highlights_overlay_png")),
      tumblerOn (resourceImage ("tumbler_on2x_png")), tumblerOff (resourceImage ("tumbler_off2x_png")),
      needle (resourceImage ("vu_needle2x_png")), glass (resourceImage ("vu_glass_overlay_png"))
{
    setSize (1032, 576);
    setResizable (true, true);
    setResizeLimits (688, 384, 2752, 1536);
    getConstrainer()->setFixedAspectRatio (1376.0 / 768.0);

    compression.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    compression.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    compression.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
    compression.setRange (0.0, 100.0, 0.01);

    makeup.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    makeup.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    makeup.setRotaryParameters (juce::degreesToRadians (-135.0f), juce::degreesToRadians (135.0f), true);
    makeup.setRange (-12.0, 24.0, 0.01);
    makeup.setDoubleClickReturnValue (true, 0.0);

    tube.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    tube.setTextBoxStyle (juce::Slider::NoTextBox, false, 0, 0);
    tube.setRange (0.0, 4.0, 1.0);
    tube.setRotaryParameters (juce::degreesToRadians (-120.0f), juce::degreesToRadians (120.0f), true);

    // These parameter-backed controls sit over the artwork and are visually transparent.
    for (auto* slider : { &compression, &makeup, &tube })
    {
        for (auto colour : { juce::Slider::rotarySliderFillColourId, juce::Slider::rotarySliderOutlineColourId,
                             juce::Slider::thumbColourId, juce::Slider::textBoxTextColourId,
                             juce::Slider::textBoxBackgroundColourId, juce::Slider::textBoxOutlineColourId })
            slider->setColour (colour, juce::Colours::transparentBlack);
        addAndMakeVisible (*slider);
    }
    power.setClickingTogglesState (true);
    power.setColour (juce::ToggleButton::textColourId, juce::Colours::transparentBlack);
    power.setColour (juce::ToggleButton::tickColourId, juce::Colours::transparentBlack);
    power.setColour (juce::ToggleButton::tickDisabledColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (power);
    startTimerHz (30);
}

juce::Rectangle<int> KomprussrAudioProcessorEditor::scaledBounds (int x, int y, int width, int height) const
{
    const auto scale = juce::jmin (getWidth() / 1376.0f, getHeight() / 768.0f);
    const auto left = (getWidth() - 1376.0f * scale) * 0.5f;
    const auto top = (getHeight() - 768.0f * scale) * 0.5f;
    return { juce::roundToInt (left + x * scale), juce::roundToInt (top + y * scale),
             juce::roundToInt (width * scale), juce::roundToInt (height * scale) };
}

float KomprussrAudioProcessorEditor::angleForDb (float db) noexcept
{
    static constexpr float points[][2] = { { -20.0f, -62.0f }, { -10.0f, -43.7f }, { -7.0f, -31.3f },
        { -5.0f, -19.5f }, { -3.0f, -3.9f }, { -1.0f, 16.3f }, { 0.0f, 28.1f },
        { 1.0f, 40.9f }, { 2.0f, 54.2f }, { 3.0f, 67.8f } };
    db = juce::jlimit (-20.0f, 3.0f, db);
    for (int i = 1; i < 10; ++i)
        if (db <= points[i][0])
        {
            const auto proportion = (db - points[i - 1][0]) / (points[i][0] - points[i - 1][0]);
            return points[i - 1][1] + proportion * (points[i][1] - points[i - 1][1]);
        }
    return points[9][1];
}

void KomprussrAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xff171311));
    const auto scale = juce::jmin (getWidth() / 1376.0f, getHeight() / 768.0f);
    const auto left = (getWidth() - 1376.0f * scale) * 0.5f;
    const auto top = (getHeight() - 768.0f * scale) * 0.5f;
    g.saveState();
    g.addTransform (juce::AffineTransform::scale (scale).translated (left, top));

    drawLayer (g, panel, 0, 0, 1376, 768);
    drawLayer (g, scales, 0, 0, 1376, 768);
    g.setOpacity (lampOpacity); drawLayer (g, tubeGlow, 460, 472, 283, 193); drawLayer (g, lamp, 1076, 53, 92, 92);
    g.setOpacity (1.0f);
    drawLayer (g, shadows, 0, 0, 1376, 768);

    const auto compressionAngle = -135.0f + 270.0f * static_cast<float> (compression.getValue() / 100.0);
    const auto makeupAngle = -135.0f + 270.0f * static_cast<float> ((makeup.getValue() + 12.0) / 36.0);
    static constexpr float tubeAngles[] = { -120.0f, -60.0f, 0.0f, 60.0f, 120.0f };
    const auto tubeIndex = juce::jlimit (0, 4, juce::roundToInt (static_cast<float> (tube.getValue())));
    drawRotatedLayer (g, compressionImage, 121, 347, 250, 250, 246, 472, compressionAngle);
    drawRotatedLayer (g, makeupImage, 1005, 347, 250, 250, 1130, 472, makeupAngle);
    drawRotatedLayer (g, tubeImage, 806, 517, 120, 120, 866, 577, tubeAngles[tubeIndex]);
    drawRotatedHighlight (g, highlights, 121, 347, 250, 250, 246, 472, compressionAngle);
    drawRotatedHighlight (g, highlights, 1005, 347, 250, 250, 1130, 472, makeupAngle);
    drawRotatedHighlight (g, highlights, 806, 517, 120, 120, 866, 577, tubeAngles[tubeIndex]);
    g.saveState();
    g.reduceClipRegion (juce::Rectangle<int> (1192, 107, 100, 100));
    drawLayer (g, highlights, 0, 0, 1376, 768);
    g.restoreState();
    const auto isOn = power.getToggleState();
    drawLayer (g, isOn ? tumblerOn : tumblerOff, 1192, 107, 100, 100);

    g.saveState();
    juce::Path circle; circle.addEllipse (683.2f - 258.4f, 468.9f - 258.4f, 516.8f, 516.8f);
    g.reduceClipRegion (circle);
    g.reduceClipRegion (juce::Rectangle<int> (458, 165, 463, 211));
    drawRotatedLayer (g, needle, 459, 132, 480, 480, 699, 372, angleForDb (needleDb));
    g.restoreState();
    drawLayer (g, glass, 458, 165, 463, 211);
    g.restoreState();
}

void KomprussrAudioProcessorEditor::resized()
{
    compression.setBounds (scaledBounds (121, 347, 250, 250));
    makeup.setBounds (scaledBounds (1005, 347, 250, 250));
    tube.setBounds (scaledBounds (806, 517, 120, 120));
    power.setBounds (scaledBounds (1192, 107, 100, 100));
}

void KomprussrAudioProcessorEditor::timerCallback()
{
    const auto now = juce::Time::getMillisecondCounterHiRes();
    const auto deltaSeconds = lastTickMs > 0.0 ? juce::jlimit (0.0, 0.1, (now - lastTickMs) / 1000.0) : 1.0 / 30.0;
    lastTickMs = now;
    const auto targetOpacity = power.getToggleState() ? 1.0f : 0.0f;
    const auto fadeStep = static_cast<float> (deltaSeconds / 1.5);
    lampOpacity += juce::jlimit (-fadeStep, fadeStep, targetOpacity - lampOpacity);
    const auto targetDb = power.getToggleState()
        ? -processor.gainReductionDb.load (std::memory_order_relaxed) : 0.0f;
    const auto needleAlpha = 1.0f - std::exp (-static_cast<float> (deltaSeconds) / 0.3f);
    needleDb += needleAlpha * (targetDb - needleDb);
    repaint();
}
