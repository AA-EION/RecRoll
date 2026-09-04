#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace RecRoll
{

/**
 * The About panel.
 *
 * Presented as a modal overlay over the whole editor rather than as extra
 * furniture in the header or the waveform area, so the working UI is exactly
 * what it was: one button in the bottom bar opens this, and closing it puts
 * every pixel back.
 *
 * It carries the attribution the project owes: RecRoll is a product of
 * EION STUDIOS, engineered by ISSEN Software Group, and the donation route
 * that pays for AAX support and signed binaries.
 */
class AboutComponent : public juce::Component
{
public:
    AboutComponent();
    ~AboutComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void mouseUp(const juce::MouseEvent& e) override;
    bool keyPressed(const juce::KeyPress& key) override;

    /** Shows the panel over its parent and takes keyboard focus. */
    void show();

    /** Invoked when the panel wants to be dismissed. */
    std::function<void()> onDismiss;

    /** Invoked when the user asks to donate; the editor owns the actual URL. */
    std::function<void()> onDonate;

    /** Height the panel would like, clamped to whatever the editor can spare. */
    static constexpr int PREFERRED_PANEL_WIDTH  = 560;
    static constexpr int PREFERRED_PANEL_HEIGHT = 372;

private:
    /** Bounds of the card itself, inside the dimmed backdrop. */
    juce::Rectangle<int> panelBounds() const;

    std::unique_ptr<juce::Drawable> appMark;   // RecRoll's own application mark
    std::unique_ptr<juce::Drawable> eionMark;  // the EION STUDIOS symbol

    juce::TextButton donateBtn { "Donate via PayPal" };
    juce::TextButton closeBtn  { "Close" };
    juce::HyperlinkButton studioLink;
    juce::HyperlinkButton sourceLink;

    /** Draws the application mark natively if the embedded SVG cannot be parsed. */
    void paintFallbackAppMark(juce::Graphics& g, juce::Rectangle<float> area) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AboutComponent)
};

} // namespace RecRoll
