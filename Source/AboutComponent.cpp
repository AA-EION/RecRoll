#include "AboutComponent.h"
#include "BrandAssets.h"

#include <juce_audio_processors/juce_audio_processors.h>

namespace RecRoll
{

namespace
{
    // The panel borrows the editor's palette so it reads as part of the app.
    const juce::Colour kScrim       { 0xcc07080c };
    const juce::Colour kCard        { 0xff171922 };
    const juce::Colour kCardEdge    { 0xff2a2d3d };
    const juce::Colour kAccent      { 0xff00e5ff };
    const juce::Colour kBone        { 0xfff2f0eb };
    const juce::Colour kMuted       { 0xff8b949e };
    const juce::Colour kFaint       { 0xff606770 };
    const juce::Colour kAmber       { 0xffffb300 };
    const juce::Colour kAmberInk    { 0xff1a1300 };

    juce::String versionString()
    {
       #if defined(JucePlugin_VersionString)
        return juce::String(JucePlugin_VersionString);
       #else
        return "1.0.0";
       #endif
    }
}

AboutComponent::AboutComponent()
    : studioLink("eionstudios.com", juce::URL("https://eionstudios.com")),
      sourceLink("github.com/AA-EION/RecRoll", juce::URL("https://github.com/AA-EION/RecRoll"))
{
    appMark  = juce::Drawable::createFromImageData(BrandAssets::RecRoll_Icon_svg,
                                                   BrandAssets::RecRoll_Icon_svgSize);
    eionMark = juce::Drawable::createFromImageData(BrandAssets::EION_Simbolo_Blanco_svg,
                                                   BrandAssets::EION_Simbolo_Blanco_svgSize);

    donateBtn.setColour(juce::TextButton::buttonColourId, kAmber);
    donateBtn.setColour(juce::TextButton::textColourOffId, kAmberInk);
    donateBtn.setTooltip("Support RecRoll development via PayPal");
    donateBtn.onClick = [this] { if (onDonate) onDonate(); };
    addAndMakeVisible(donateBtn);

    closeBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222530));
    closeBtn.setColour(juce::TextButton::textColourOffId, kMuted);
    closeBtn.onClick = [this] { if (onDismiss) onDismiss(); };
    addAndMakeVisible(closeBtn);

    for (auto* link : { &studioLink, &sourceLink })
    {
        link->setColour(juce::HyperlinkButton::textColourId, kAccent);
        link->setJustificationType(juce::Justification::centredLeft);
        link->setFont(juce::FontOptions(12.5f), false, juce::Justification::centredLeft);
        addAndMakeVisible(*link);
    }

    setWantsKeyboardFocus(true);
    setVisible(false);
}

AboutComponent::~AboutComponent() = default;

void AboutComponent::show()
{
    setVisible(true);
    toFront(true);
    grabKeyboardFocus();
    resized();
    repaint();
}

juce::Rectangle<int> AboutComponent::panelBounds() const
{
    const int w = juce::jmin(PREFERRED_PANEL_WIDTH,  getWidth()  - 24);
    const int h = juce::jmin(PREFERRED_PANEL_HEIGHT, getHeight() - 16);
    return juce::Rectangle<int>(w, h).withCentre(getLocalBounds().getCentre());
}

void AboutComponent::mouseUp(const juce::MouseEvent& e)
{
    // Clicking the dimmed area outside the card dismisses, as a modal should.
    if (!panelBounds().contains(e.getPosition()) && onDismiss)
        onDismiss();
}

bool AboutComponent::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode(juce::KeyPress::escapeKey))
    {
        if (onDismiss)
            onDismiss();
        return true;
    }

    // Swallow everything else so the editor's Space/Cmd-A shortcuts do not
    // fire at an audio buffer the user cannot currently see.
    return true;
}

void AboutComponent::paintFallbackAppMark(juce::Graphics& g, juce::Rectangle<float> area) const
{
    const float s = juce::jmin(area.getWidth(), area.getHeight());
    auto square = juce::Rectangle<float>(s, s).withCentre(area.getCentre());

    g.setColour(juce::Colour(0xff11141c));
    g.fillRoundedRectangle(square, s * 0.225f);

    juce::Path ring;
    ring.addCentredArc(square.getCentreX(), square.getCentreY(),
                       s * 0.333f, s * 0.333f, 0.0f,
                       juce::degreesToRadians(112.0f), juce::degreesToRadians(390.0f), true);
    g.setColour(kAccent);
    g.strokePath(ring, juce::PathStrokeType(s * 0.077f, juce::PathStrokeType::curved,
                                            juce::PathStrokeType::rounded));

    const float barW = s * 0.062f;
    const float gap  = s * 0.044f;
    const float hs[] = { 0.30f, 0.62f, 1.00f, 0.62f, 0.30f };
    float x = square.getCentreX() - (5 * barW + 4 * gap) * 0.5f;
    for (int i = 0; i < 5; ++i)
    {
        const float half = hs[i] * s * 0.185f;
        g.setColour(i == 2 ? kBone : kAccent);
        g.fillRoundedRectangle(x, square.getCentreY() - half, barW, half * 2.0f, barW * 0.5f);
        x += barW + gap;
    }
}

void AboutComponent::paint(juce::Graphics& g)
{
    g.fillAll(kScrim);

    auto card = panelBounds().toFloat();
    g.setColour(kCard);
    g.fillRoundedRectangle(card, 10.0f);
    g.setColour(kCardEdge);
    g.drawRoundedRectangle(card, 10.0f, 1.0f);

    auto body = card.reduced(22.0f, 18.0f);

    // --- identity row ------------------------------------------------------
    auto header = body.removeFromTop(64.0f);
    auto markArea = header.removeFromLeft(64.0f);
    if (appMark != nullptr)
        appMark->drawWithin(g, markArea, juce::RectanglePlacement::centred, 1.0f);
    else
        paintFallbackAppMark(g, markArea);

    header.removeFromLeft(14.0f);

    g.setColour(kAccent);
    g.setFont(juce::FontOptions(24.0f).withKerningFactor(0.12f));
    g.drawText("RECROLL", header.removeFromTop(30.0f), juce::Justification::bottomLeft, false);

    g.setColour(kMuted);
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("Version " + versionString() + "  \xe2\x80\xa2  Rolling sampler & retrospective recorder",
               header.removeFromTop(20.0f), juce::Justification::topLeft, false);

    body.removeFromTop(14.0f);
    g.setColour(kCardEdge);
    g.fillRect(body.removeFromTop(1.0f));
    body.removeFromTop(14.0f);

    // --- who makes it ------------------------------------------------------
    auto credits = body.removeFromTop(46.0f);
    auto eionArea = credits.removeFromLeft(40.0f).reduced(0.0f, 3.0f);
    if (eionMark != nullptr)
        eionMark->drawWithin(g, eionArea, juce::RectanglePlacement::centred, 1.0f);

    credits.removeFromLeft(12.0f);
    g.setColour(kBone);
    g.setFont(juce::FontOptions(13.5f).withKerningFactor(0.06f));
    g.drawText("A product of EION STUDIOS", credits.removeFromTop(19.0f),
               juce::Justification::topLeft, false);

    g.setColour(kMuted);
    g.setFont(juce::FontOptions(12.5f));
    g.drawText("Engineered by ISSEN Software Group", credits.removeFromTop(18.0f),
               juce::Justification::topLeft, false);

    // The studio link is a real button; reserve its row here.
    body.removeFromTop(22.0f);

    body.removeFromTop(10.0f);
    g.setColour(kCardEdge);
    g.fillRect(body.removeFromTop(1.0f));
    body.removeFromTop(12.0f);

    // --- why donating matters ---------------------------------------------
    g.setColour(kAmber);
    g.setFont(juce::FontOptions(12.5f).withKerningFactor(0.06f));
    g.drawText("SUPPORT RECROLL", body.removeFromTop(17.0f), juce::Justification::topLeft, false);

    g.setColour(kMuted);
    g.setFont(juce::FontOptions(12.0f));
    g.drawFittedText("RecRoll is free and open source. Donations go directly towards AAX (Pro Tools) "
                     "support and towards code-signed and notarised macOS and Windows binaries, so "
                     "the installers stop tripping Gatekeeper and SmartScreen.",
                     body.removeFromTop(48.0f).toNearestInt(), juce::Justification::topLeft, 3, 1.0f);

    // Donate button and source link occupy the next rows; laid out in resized().
    body.removeFromTop(34.0f + 10.0f + 20.0f);

    // --- footer ------------------------------------------------------------
    g.setColour(kFaint);
    g.setFont(juce::FontOptions(11.0f));
    g.drawText(juce::String::fromUTF8("\xc2\xa9 2026 EION STUDIOS  \xe2\x80\xa2  GNU GPL v3.0"),
               card.reduced(22.0f, 14.0f).removeFromBottom(15.0f),
               juce::Justification::bottomLeft, false);
}

void AboutComponent::resized()
{
    auto card = panelBounds();
    auto body = card.reduced(22, 18);

    closeBtn.setBounds(card.getRight() - 22 - 68, card.getY() + 14, 68, 24);

    // Mirror paint()'s vertical walk so the interactive rows land on the gaps
    // the drawing left for them.
    body.removeFromTop(64);          // identity row
    body.removeFromTop(14 + 1 + 14); // rule
    body.removeFromTop(46);          // credits block

    studioLink.setBounds(body.removeFromTop(22).withTrimmedLeft(52).withWidth(220));

    body.removeFromTop(10 + 1 + 12); // rule
    body.removeFromTop(17);          // "SUPPORT RECROLL"
    body.removeFromTop(48);          // support paragraph

    auto actionRow = body.removeFromTop(34);
    donateBtn.setBounds(actionRow.removeFromLeft(184).reduced(0, 2));

    body.removeFromTop(10);
    sourceLink.setBounds(body.removeFromTop(20).withWidth(260));
}

} // namespace RecRoll
