#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "WaveformComponent.h"
#include "AboutComponent.h"

namespace RecRoll
{

class RecRollAudioProcessorEditor : public juce::AudioProcessorEditor,
                                    public juce::Button::Listener
{
public:
    explicit RecRollAudioProcessorEditor(RecRollAudioProcessor&);
    ~RecRollAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    void buttonClicked(juce::Button* button) override;
    bool keyPressed(const juce::KeyPress& key) override;

private:
    RecRollAudioProcessor& processorRef;

    // Subcomponents
    WaveformComponent waveform;

    // Header Controls
    juce::Label titleLabel;
    juce::TextButton duration15sBtn  { "15s" };
    juce::TextButton duration30sBtn  { "30s" };
    juce::TextButton duration60sBtn  { "1m" };
    juce::TextButton duration120sBtn { "2m" };
    juce::TextButton duration300sBtn { "5m" };
    juce::TextButton duration600sBtn { "10m" };

    juce::ToggleButton freezeBtn     { "Freeze" };
    juce::TextButton clearBtn        { "Clear" };
    juce::TextButton auditionBtn     { "Audition" };
    juce::ToggleButton normalizeBtn  { "Normalize" };
    juce::ToggleButton muteThruBtn   { "Mute Thru" };
    juce::TextButton donateHeaderBtn { "Donate" };

    // Session Donation Banner (Appears each time editor is loaded)
    struct DonationBanner : public juce::Component
    {
        juce::Label messageLabel;
        juce::TextButton donateBtn   { "Donate via PayPal" };
        juce::TextButton dismissBtn  { "X" };

        DonationBanner();
        void paint(juce::Graphics& g) override;
        void resized() override;
    } donationBanner;

    // Bottom Bar Controls & Labels
    juce::Label timeSelectionLabel;
    juce::TextButton dragAudioBtn    { "DRAG AUDIO TO DAW" };

    /**
     * Turns a drag starting on the "DRAG AUDIO TO DAW" button into a real OS
     * drag.
     *
     * The button used to forward its raw mouse events to the waveform, but a
     * MouseEvent is expressed in the coordinate space of the component it
     * happened on - so button-local x values were being read as waveform
     * positions. That re-anchored the selection to a meaningless point on every
     * press, and the waveform's mouseUp then saw a zero-width selection and
     * expanded it to the whole buffer. The button consequently exported
     * everything and never the actual selection.
     */
    struct DragButtonListener : public juce::MouseListener
    {
        explicit DragButtonListener(WaveformComponent& w) : waveform(w) {}

        void mouseDown(const juce::MouseEvent&) override { startedDrag = false; }

        void mouseDrag(const juce::MouseEvent& e) override
        {
            if (!startedDrag && e.getDistanceFromDragStart() > 8)
            {
                startedDrag = true;
                waveform.startDawDragOperation();
            }
        }

        WaveformComponent& waveform;

        /** Set once a drag has been handed to the OS, so the button's own click
            callback does not export the same selection a second time. Cleared on
            the next press, which always precedes that callback. */
        bool startedDrag { false };
    };

    DragButtonListener dragButtonListener { waveform };
    juce::Label statusInfoLabel;
    juce::TextButton aboutBtn        { "About" };
    juce::TextButton uninstallBtn    { "Uninstall..." };

    // About overlay - hidden until requested, so the working layout is untouched
    AboutComponent aboutPanel;

    // APVTS Attachments
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> freezeAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> muteThruAttachment;
    std::unique_ptr<juce::AudioProcessorValueTreeState::ButtonAttachment> normalizeAttachment;

    void openDonationLink();
    void updateDurationButtonStyles(double activeSeconds);
    void setDurationChoice(int choiceIndex);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecRollAudioProcessorEditor)
};

} // namespace RecRoll
