#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "WaveformComponent.h"

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
    juce::Label statusInfoLabel;
    juce::TextButton uninstallBtn    { "Uninstall..." };

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
