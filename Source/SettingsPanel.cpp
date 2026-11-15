#include "SettingsPanel.h"
#include "UpdateChecker.h"
#include <vector>
#include <utility>

namespace
{
// All editable product text and links are kept together here.
constexpr auto creatorName = "Pedepena";
constexpr auto productName = "KOMPRUSSR KU-01";
constexpr auto instagramUrl = "https://instagram.com/pedepenaprod";
constexpr auto instagramText = "Instagram: @pedepenaprod";
constexpr auto emailUrl = "mailto:pedepenaprod@gmail.com";
constexpr auto emailText = "Email: pedepenaprod@gmail.com";
constexpr auto repositoryUrl = "https://github.com/pedepena/komprussr";
constexpr auto releasesUrl = "https://github.com/pedepena/komprussr/releases";
constexpr auto releaseDate = "15/11/2026";
constexpr auto releaseDatePrefix = "Release date: ";
constexpr auto createdByPrefix = "Created by: ";
constexpr auto copyrightPrefix = "© 2026 ";
constexpr auto checkUpdatesText = "(check updates)";
constexpr auto sourceCodeText = "Source code";
constexpr auto licenseText = "Licensed under the GNU AGPL v3";
constexpr auto juceText = "Made with JUCE (used under the terms of the AGPLv3).";
// Pending verification against Steinberg's trademark usage rules.
constexpr auto vstTrademarkText = "VST is a trademark of Steinberg Media Technologies GmbH, registered in Europe and other countries.";
constexpr auto warrantyText = "Provided \"as is\", without warranty of any kind.";
constexpr auto nonAffiliationText = "Independent project, not affiliated with any equipment or vacuum tube manufacturer.";
constexpr auto manualTitle = "KOMPRUSSR KU-01 — Quick manual";
constexpr auto manualSections[][2] = {
    { "What it is", "KOMPRUSSR is a \"vari-mu\" style tube compressor. A compressor turns down the loud parts of a sound so everything feels more even and glued together. KOMPRUSSR also adds a touch of warm tube colour. It is inspired by classic tube designs; it is not an exact copy of any real device." },
    { "Controls", "COMPRESSION (0–100 %)\nHow much the sound gets squeezed. Near 0 % there is practically no compression. As you turn it up, the loud parts are turned down more and more, and you also hear more tube colour. Start low and raise it until you like what you hear.\n\nMAKE UP GAIN (−12 to +24 dB)\nCompressing makes the overall sound quieter. This knob brings the volume back up. Double-click it to return to 0 dB.\n\nTUBE (5 positions)\nChooses the character of the compressor (see \"The five tubes\" below). You can change it while the music plays, with no volume jumps.\n\nPOWER (switch)\nON: the compressor is working, and the lamp and tubes glow. OFF: the sound passes through untouched.\n\nGR meter\n\"GR\" means Gain Reduction: how much the compressor is turning the sound down right now. With the needle at 0, nothing is happening. The further it moves to the left, the more it is compressing. A few dB of reduction is usually plenty." },
    { "The five tubes", "Each tube changes how fast the compressor reacts, how firmly it squeezes and what kind of colour it adds. The differences are subtle but real.\n\n6386U — The smoothest and cleanest. Gentle and slow, like glue. Great for full mixes and anything you want to keep natural.\n\n6BA6 — The all-rounder. Balanced body, balanced speed.\n\n6BC8 — Faster and brighter, with a bit more bite. Brings energy to drums and lively sounds.\n\n6K7 — Thicker and rounder, a little darker. Adds weight.\n\n6SK7 — The firmest and most colourful, and the fastest to react. The most obvious effect." },
    { "Tips", "Start with Compression low and watch the GR meter. If the needle barely moves, you are hardly compressing.\n\nAfter compressing, raise Make Up Gain until the volume matches the original sound. Louder usually sounds \"better\", so match the volume before judging.\n\nUse Power to switch between the original and the compressed sound.\n\nTry each tube on the same sound and keep the one that feels right." },
    { "Good to know", "Works in mono and stereo (VST3). All controls can be automated and are saved with your project. The plugin reports a small delay (latency) to your DAW, which compensates for it automatically." }
};
const auto gold = juce::Colour (0xffbe8f5c);
const auto lightGold = juce::Colour (0xffd9ae72);
const auto panelRed = juce::Colour (0xff7a160f);
const auto textColour = juce::Colour (0xffe7dfd4);

class PanelLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour&, bool hovered, bool down) override
    {
        auto bounds = button.getLocalBounds().toFloat().reduced (1.0f);
        g.setColour (button.getToggleState() || down ? panelRed : juce::Colour (0xff281d18));
        g.fillRoundedRectangle (bounds, 3.0f);
        g.setColour (hovered ? lightGold : gold);
        g.drawRoundedRectangle (bounds, 3.0f, 1.2f);
    }
    void drawButtonText (juce::Graphics& g, juce::TextButton& button, bool hovered, bool) override
    {
        g.setColour (hovered || button.getToggleState() ? lightGold : gold);
        g.setFont (juce::Font (juce::jmax (10.0f, button.getHeight() * 0.36f), juce::Font::bold));
        g.drawFittedText (button.getButtonText(), button.getLocalBounds().reduced (3), juce::Justification::centred, 1);
    }
};

PanelLookAndFeel panelLookAndFeel;
}

class SettingsPanel::AboutContent final : public juce::Component
{
public:
    AboutContent()
    {
        addText (juce::String (productName) + "  v" + UpdateChecker::currentVersion(), 0, true);
        addLink (checkUpdatesText, releasesUrl, 1);
        addText (juce::String (releaseDatePrefix) + releaseDate, 2);
        addText (juce::String (createdByPrefix) + creatorName, 3);
        addLink (instagramText, instagramUrl, 4);
        addLink (emailText, emailUrl, 5);
        addText (juce::String (copyrightPrefix) + creatorName, 6);
        addText (licenseText, 7);
        addLink (sourceCodeText, repositoryUrl, 8);
        addText (juceText, 9);
        addText (vstTrademarkText, 10);
        addText (warrantyText, 11);
        addText (nonAffiliationText, 12);
        setSize (700, 660);
    }
    void setScale (float s, int width) { scale = s; setSize (width, getHeight()); resized(); }
    void resized() override
    {
        const auto row = juce::roundToInt (34.0f * scale);
        for (auto& item : labels)
        {
            const auto height = (item.first == 10 || item.first == 12) ? juce::roundToInt (54.0f * scale) : row;
            auto y = item.first * row;
            if (item.first > 10) y += juce::roundToInt (40.0f * scale);
            if (item.first > 12) y += juce::roundToInt (40.0f * scale);
            item.second->setBounds (0, y, getWidth(), height);
            item.second->setFont (juce::Font (juce::jmax (13.0f, (item.first == 0 ? 23.0f : 17.0f) * scale), item.first == 0 ? juce::Font::bold : juce::Font::plain));
            item.second->setMinimumHorizontalScale (0.72f);
        }
        for (auto& link : links)
        {
            const auto rowIndex = link.first;
            link.second->setBounds (0, rowIndex * row, getWidth(), row);
            link.second->setFont (juce::Font (juce::jmax (13.0f, 17.0f * scale)), false);
        }
        setSize (getWidth(), 13 * row + juce::roundToInt (108.0f * scale));
    }
private:
    void addText (const juce::String& value, int row, bool heading = false)
    {
        auto label = std::make_unique<juce::Label>();
        label->setText (value, juce::dontSendNotification);
        label->setColour (juce::Label::textColourId, heading ? gold : textColour);
        label->setFont (juce::Font (heading ? 23.0f : 17.0f));
        label->setJustificationType (juce::Justification::centredLeft);
        addAndMakeVisible (*label);
        labels.emplace_back (row, std::move (label));
    }
    void addLink (const juce::String& text, const juce::String& url, int row)
    {
        auto link = std::make_unique<juce::HyperlinkButton> (text, juce::URL (url));
        link->setColour (juce::HyperlinkButton::textColourId, gold);
        link->setMouseCursor (juce::MouseCursor::pointingHandCursor);
        addAndMakeVisible (*link);
        links.emplace_back (row, std::move (link));
    }
    float scale = 1.0f;
    std::vector<std::pair<int, std::unique_ptr<juce::Label>>> labels;
    std::vector<std::pair<int, std::unique_ptr<juce::HyperlinkButton>>> links;
};

class SettingsPanel::ManualContent final : public juce::Component
{
public:
    void setScale (float s, int width)
    {
        scale = s;
        setSize (width, 1);
        float y = 12.0f * scale;
        const auto widthF = juce::jmax (100.0f, static_cast<float> (getWidth()) - 24.0f * scale);
        juce::TextLayout title;
        title.createLayout (manualTitle, juce::Font (juce::jmax (16.0f, 32.0f * scale), juce::Font::bold), widthF);
        y += title.getHeight() + 15.0f * scale;
        for (const auto& section : manualSections)
        {
            juce::TextLayout heading;
            heading.createLayout (section[0], juce::Font (juce::jmax (14.0f, 27.0f * scale), juce::Font::bold), widthF);
            y += heading.getHeight() + 5.0f * scale;
            juce::TextLayout body;
            body.createLayout (section[1], juce::Font (juce::jmax (13.0f, 24.0f * scale)), widthF);
            y += body.getHeight() + 16.0f * scale;
        }
        setSize (width, juce::roundToInt (y));
    }
    void paint (juce::Graphics& g) override
    {
        float y = 12.0f * scale;
        const auto width = juce::jmax (100.0f, static_cast<float> (getWidth()) - 24.0f * scale);
        juce::TextLayout title;
        title.createLayout (manualTitle, juce::Font (juce::jmax (16.0f, 32.0f * scale), juce::Font::bold), width);
        g.setColour (gold); title.draw (g, { 12.0f * scale, y, width, title.getHeight() });
        y += title.getHeight() + 15.0f * scale;
        for (const auto& section : manualSections)
        {
            juce::TextLayout heading;
            heading.createLayout (section[0], juce::Font (juce::jmax (14.0f, 27.0f * scale), juce::Font::bold), width);
            g.setColour (gold); heading.draw (g, { 12.0f * scale, y, width, heading.getHeight() });
            y += heading.getHeight() + 5.0f * scale;
            juce::TextLayout body;
            body.createLayout (section[1], juce::Font (juce::jmax (13.0f, 24.0f * scale)), width);
            g.setColour (textColour); body.draw (g, { 12.0f * scale, y, width, body.getHeight() });
            y += body.getHeight() + 16.0f * scale;
        }
    }
private:
    float scale = 1.0f;
};

SettingsPanel::SettingsPanel (std::function<void()> closeFn) : closeRequested (std::move (closeFn))
{
    setInterceptsMouseClicks (true, true);
    for (auto* button : { &aboutTab, &manualTab, &settingsTab, &closeButton, &downloadButton, &updateCloseButton })
    {
        button->setColour (juce::TextButton::buttonColourId, juce::Colour (0xff281d18));
        button->setColour (juce::TextButton::buttonOnColourId, panelRed);
        button->setColour (juce::TextButton::textColourOffId, gold);
        button->setColour (juce::TextButton::textColourOnId, lightGold);
        button->setLookAndFeel (&panelLookAndFeel);
        addAndMakeVisible (*button);
    }
    aboutTab.onClick = [this] { selectTab (0); };
    manualTab.onClick = [this] { selectTab (1); };
    settingsTab.onClick = [this] { selectTab (2); };
    closeButton.onClick = [this] { close(); };
    downloadButton.onClick = [this] { openLink (pendingUrl); };
    updateCloseButton.onClick = [this] { showingUpdate = false; close(); };
    aboutContent = std::make_unique<AboutContent>();
    manualContent = std::make_unique<ManualContent>();
    viewport.setScrollBarsShown (true, false);
    viewport.setViewedComponent (aboutContent.get(), false);
    viewport.setColour (juce::ScrollBar::thumbColourId, gold.withAlpha (0.65f));
    viewport.setColour (juce::ScrollBar::trackColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (viewport);
    automaticChecks.setToggleState (UpdateChecker::automaticChecksEnabled(), juce::dontSendNotification);
    automaticChecks.setColour (juce::ToggleButton::textColourId, textColour);
    automaticChecks.setColour (juce::ToggleButton::tickColourId, gold);
    automaticChecks.onClick = [this] { UpdateChecker::setAutomaticChecksEnabled (automaticChecks.getToggleState()); };
    addAndMakeVisible (automaticChecks);
    privacyNotice.setText ("When enabled, KOMPRUSSR contacts github.com to look for a newer version (GitHub sees your IP address).", juce::dontSendNotification);
    privacyNotice.setColour (juce::Label::textColourId, textColour.withAlpha (0.8f));
    privacyNotice.setFont (juce::Font (15.0f));
    addAndMakeVisible (privacyNotice);
    updateMessage.setJustificationType (juce::Justification::centred);
    updateMessage.setColour (juce::Label::textColourId, textColour);
    addAndMakeVisible (updateMessage);
    selectTab (0);
}

SettingsPanel::~SettingsPanel()
{
    for (auto* button : { &aboutTab, &manualTab, &settingsTab, &closeButton, &downloadButton, &updateCloseButton })
        button->setLookAndFeel (nullptr);
    viewport.setViewedComponent (nullptr, false);
}

void SettingsPanel::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colours::black.withAlpha (0.64f));
    auto frame = getLocalBounds().toFloat().reduced (getWidth() * 0.08f, getHeight() * 0.09f);
    g.setColour (juce::Colour (0xf0211814)); g.fillRoundedRectangle (frame, 8.0f);
    g.setColour (panelRed); g.drawRoundedRectangle (frame, 8.0f, 2.0f);
    if (showingUpdate)
    {
        g.setColour (juce::Colour (0xf0211814)); g.fillRoundedRectangle (frame.reduced (frame.getWidth() * 0.14f, frame.getHeight() * 0.28f), 8.0f);
        g.setColour (gold); g.drawRoundedRectangle (frame.reduced (frame.getWidth() * 0.14f, frame.getHeight() * 0.28f), 8.0f, 2.0f);
    }
}

void SettingsPanel::resized()
{
    const auto scale = juce::jmin (getWidth() / 1376.0f, getHeight() / 768.0f);
    const auto frame = getLocalBounds().reduced (juce::roundToInt (getWidth() * 0.08f), juce::roundToInt (getHeight() * 0.09f));
    const auto pad = juce::roundToInt (22.0f * scale);
    const auto headerHeight = juce::roundToInt (52.0f * scale);
    const auto closeSize = juce::roundToInt (38.0f * scale);
    closeButton.setBounds (frame.getRight() - closeSize - pad / 2, frame.getY() + pad / 2, closeSize, closeSize);
    const auto tabWidth = juce::roundToInt (150.0f * scale);
    aboutTab.setBounds (frame.getX() + pad, frame.getY() + pad, tabWidth, headerHeight);
    manualTab.setBounds (aboutTab.getRight() + juce::roundToInt (8.0f * scale), frame.getY() + pad, tabWidth, headerHeight);
    settingsTab.setBounds (manualTab.getRight() + juce::roundToInt (8.0f * scale), frame.getY() + pad, tabWidth, headerHeight);
    viewport.setBounds (frame.getX() + pad, aboutTab.getBottom() + pad / 2, frame.getWidth() - 2 * pad, frame.getHeight() - headerHeight - 2 * pad);
    aboutContent->setScale (scale, viewport.getWidth() - juce::roundToInt (16.0f * scale));
    manualContent->setScale (scale, viewport.getWidth() - juce::roundToInt (20.0f * scale));
    automaticChecks.setBounds (viewport.getX() + pad, viewport.getY() + pad, viewport.getWidth() - 2 * pad, juce::roundToInt (42.0f * scale));
    privacyNotice.setBounds (automaticChecks.getX(), automaticChecks.getBottom() + pad / 2, automaticChecks.getWidth(), juce::roundToInt (68.0f * scale));
    updateMessage.setBounds (frame.getX() + pad * 2, frame.getCentreY() - pad * 2, frame.getWidth() - 4 * pad, juce::roundToInt (65.0f * scale));
    downloadButton.setBounds (frame.getCentreX() - juce::roundToInt (130.0f * scale), updateMessage.getBottom(), juce::roundToInt (120.0f * scale), juce::roundToInt (44.0f * scale));
    updateCloseButton.setBounds (downloadButton.getRight() + juce::roundToInt (12.0f * scale), downloadButton.getY(), juce::roundToInt (120.0f * scale), downloadButton.getHeight());
    const auto showNormal = ! showingUpdate;
    for (auto* c : { static_cast<juce::Component*> (&aboutTab), &manualTab, &settingsTab, &closeButton, &viewport }) c->setVisible (showNormal);
    automaticChecks.setVisible (showNormal && activeTab == 2);
    privacyNotice.setVisible (showNormal && activeTab == 2);
    updateMessage.setVisible (showingUpdate); downloadButton.setVisible (showingUpdate); updateCloseButton.setVisible (showingUpdate);
}

void SettingsPanel::selectTab (int tab)
{
    activeTab = tab;
    aboutTab.setToggleState (tab == 0, juce::dontSendNotification);
    manualTab.setToggleState (tab == 1, juce::dontSendNotification);
    settingsTab.setToggleState (tab == 2, juce::dontSendNotification);
    viewport.setViewedComponent (tab == 1 ? static_cast<juce::Component*> (manualContent.get()) : aboutContent.get(), false);
    viewport.setVisible (tab != 2);
    automaticChecks.setVisible (tab == 2); privacyNotice.setVisible (tab == 2);
    resized(); repaint();
}

void SettingsPanel::showUpdateNotice (const juce::String& version, const juce::String& url)
{
    pendingUrl = url;
    updateMessage.setText ("A new version of KOMPRUSSR KU-01 is available: " + version
        + " (you have v" + UpdateChecker::currentVersion() + ")", juce::dontSendNotification);
    showingUpdate = true; setVisible (true); toFront (true); resized(); repaint();
}

void SettingsPanel::close() { showingUpdate = false; if (closeRequested) closeRequested(); }
void SettingsPanel::openLink (const juce::String& url) { if (url.isNotEmpty()) juce::URL (url).launchInDefaultBrowser(); }

void SettingsPanel::mouseDown (const juce::MouseEvent& event)
{
    const auto frame = getLocalBounds().reduced (juce::roundToInt (getWidth() * 0.08f), juce::roundToInt (getHeight() * 0.09f));
    if (! frame.contains (event.getPosition())) close();
}
