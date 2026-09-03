#include "PluginEditor.h"
#include "UninstallerHelper.h"
#include <iomanip>
#include <sstream>

namespace RecRoll
{

RecRollAudioProcessorEditor::RecRollAudioProcessorEditor(RecRollAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), waveform(p.getRollingBuffer())
{
    // Make window nicely resizable
    setResizable(true, true);
    setResizeLimits(700, 380, 1920, 1200);
    setSize(860, 460);

    // Title Label
    titleLabel.setText("RECROLL", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(20.0f).withExtraKerningFactor(0.12f));
    titleLabel.setColour(juce::Label::textColourId, juce::Colour(0xff00e5ff));
    addAndMakeVisible(titleLabel);

    // Duration buttons
    auto setupDurationBtn = [this](juce::TextButton& btn)
    {
        btn.addListener(this);
        btn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff222530));
        btn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0b8c4));
        addAndMakeVisible(btn);
    };

    setupDurationBtn(duration15sBtn);
    setupDurationBtn(duration30sBtn);
    setupDurationBtn(duration60sBtn);
    setupDurationBtn(duration120sBtn);
    setupDurationBtn(duration300sBtn);
    setupDurationBtn(duration600sBtn);

    // Action buttons
    freezeBtn.setButtonText("Freeze");
    freezeBtn.setColour(juce::ToggleButton::textColourId, juce::Colour(0xffe0e6ed));
    addAndMakeVisible(freezeBtn);

    clearBtn.setButtonText("Clear");
    clearBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2228));
    clearBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffff5252));
    clearBtn.addListener(this);
    addAndMakeVisible(clearBtn);

    auditionBtn.setButtonText("Audition (Space)");
    auditionBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff223028));
    auditionBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff00e676));
    auditionBtn.addListener(this);
    addAndMakeVisible(auditionBtn);

    normalizeBtn.setButtonText("Norm");
    normalizeBtn.setTooltip("Normalize exported audio slice to -0.1 dBFS");
    normalizeBtn.addListener(this);
    addAndMakeVisible(normalizeBtn);

    muteThruBtn.setButtonText("Mute Thru");
    muteThruBtn.setTooltip("Mute incoming audio passthrough to hear only audition playback");
    addAndMakeVisible(muteThruBtn);

    // Waveform display
    addAndMakeVisible(waveform);

    waveform.onSelectionChanged = [this](double startSec, double endSec, double durationSec)
    {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(2);
        ss << "Start: " << startSec << "s  |  End: " << endSec << "s  |  Length: " << durationSec << "s";
        timeSelectionLabel.setText(ss.str(), juce::dontSendNotification);
    };

    // Bottom bar controls
    timeSelectionLabel.setText("Select audio or drag selection directly into your DAW", juce::dontSendNotification);
    timeSelectionLabel.setFont(juce::FontOptions(12.5f));
    timeSelectionLabel.setColour(juce::Label::textColourId, juce::Colour(0xff8b949e));
    addAndMakeVisible(timeSelectionLabel);

    dragAudioBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff00bcd4));
    dragAudioBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff051016));
    dragAudioBtn.setTooltip("Click and drag from here directly into your DAW track or desktop");
    dragAudioBtn.addMouseListener(&waveform, false);
    dragAudioBtn.onClick = [this]() { waveform.startDawDragOperation(); };
    addAndMakeVisible(dragAudioBtn);

    juce::String srStr = juce::String(processorRef.getSampleRate() / 1000.0, 1) + " kHz";
    statusInfoLabel.setText(srStr + " | Clean Passthrough", juce::dontSendNotification);
    statusInfoLabel.setFont(juce::FontOptions(11.5f));
    statusInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff606770));
    statusInfoLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statusInfoLabel);

    // Uninstall button
    uninstallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2029));
    uninstallBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    uninstallBtn.setTooltip("Completely uninstall RecRoll plugins and application from system");
    uninstallBtn.addListener(this);
    addAndMakeVisible(uninstallBtn);

    // Connect APVTS attachments
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "recordingActive", freezeBtn);

    muteThruAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "passthroughMuted", muteThruBtn);

    normalizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "normalizeExport", normalizeBtn);

    updateDurationButtonStyles(processorRef.getRollingBuffer().getVisibleDurationSeconds());
}

RecRollAudioProcessorEditor::~RecRollAudioProcessorEditor()
{
    clearBtn.removeListener(this);
    auditionBtn.removeListener(this);
    normalizeBtn.removeListener(this);
    uninstallBtn.removeListener(this);
    duration15sBtn.removeListener(this);
    duration30sBtn.removeListener(this);
    duration60sBtn.removeListener(this);
    duration120sBtn.removeListener(this);
    duration300sBtn.removeListener(this);
    duration600sBtn.removeListener(this);
}

void RecRollAudioProcessorEditor::updateDurationButtonStyles(double activeSeconds)
{
    auto setHighlight = [activeSeconds](juce::TextButton& btn, double secs)
    {
        bool active = (std::abs(activeSeconds - secs) < 1.0);
        btn.setColour(juce::TextButton::buttonColourId, active ? juce::Colour(0xff00e5ff) : juce::Colour(0xff222530));
        btn.setColour(juce::TextButton::textColourOffId, active ? juce::Colour(0xff0a0d14) : juce::Colour(0xffb0b8c4));
    };

    setHighlight(duration15sBtn, RollingBuffer::DURATION_15S);
    setHighlight(duration30sBtn, RollingBuffer::DURATION_30S);
    setHighlight(duration60sBtn, RollingBuffer::DURATION_60S);
    setHighlight(duration120sBtn, RollingBuffer::DURATION_120S);
    setHighlight(duration300sBtn, RollingBuffer::DURATION_300S);
    setHighlight(duration600sBtn, RollingBuffer::DURATION_600S);
}

void RecRollAudioProcessorEditor::setDurationChoice(int choiceIndex)
{
    if (auto* param = processorRef.getAPVTS().getParameter("bufferDuration"))
    {
        param->setValueNotifyingHost(param->convertTo0to1(static_cast<float>(choiceIndex)));
    }

    double seconds = 60.0;
    switch (choiceIndex)
    {
        case 0: seconds = RollingBuffer::DURATION_15S; break;
        case 1: seconds = RollingBuffer::DURATION_30S; break;
        case 2: seconds = RollingBuffer::DURATION_60S; break;
        case 3: seconds = RollingBuffer::DURATION_120S; break;
        case 4: seconds = RollingBuffer::DURATION_300S; break;
        case 5: seconds = RollingBuffer::DURATION_600S; break;
        default: seconds = 60.0; break;
    }
    processorRef.getRollingBuffer().setVisibleDurationSeconds(seconds);
    updateDurationButtonStyles(seconds);
    waveform.clearSelection();
}

void RecRollAudioProcessorEditor::buttonClicked(juce::Button* button)
{
    if (button == &duration15sBtn)       setDurationChoice(0);
    else if (button == &duration30sBtn)  setDurationChoice(1);
    else if (button == &duration60sBtn)  setDurationChoice(2);
    else if (button == &duration120sBtn) setDurationChoice(3);
    else if (button == &duration300sBtn) setDurationChoice(4);
    else if (button == &duration600sBtn) setDurationChoice(5);
    else if (button == &clearBtn)
    {
        processorRef.getRollingBuffer().clear();
        waveform.clearSelection();
    }
    else if (button == &auditionBtn)
    {
        waveform.toggleAudition();
    }
    else if (button == &normalizeBtn)
    {
        waveform.setNormalizeExport(normalizeBtn.getToggleState());
    }
    else if (button == &uninstallBtn)
    {
        UninstallerHelper::promptAndExecuteUninstall(this);
    }
}

bool RecRollAudioProcessorEditor::keyPressed(const juce::KeyPress& key)
{
    if (key.isKeyCode(juce::KeyPress::spaceKey))
    {
        waveform.toggleAudition();
        return true;
    }
    if (key == juce::KeyPress('a', juce::ModifierKeys::commandModifier, 0)
     || key == juce::KeyPress('a', juce::ModifierKeys::ctrlModifier, 0))
    {
        waveform.selectAll();
        return true;
    }
    return false;
}

void RecRollAudioProcessorEditor::paint(juce::Graphics& g)
{
    // Modern dark panel background
    g.fillAll(juce::Colour(0xff121318));

    // Header divider line
    g.setColour(juce::Colour(0xff232632));
    g.drawHorizontalLine(50, 0.0f, static_cast<float>(getWidth()));

    // Bottom bar divider line
    g.drawHorizontalLine(getHeight() - 48, 0.0f, static_cast<float>(getWidth()));
}

void RecRollAudioProcessorEditor::resized()
{
    auto area = getLocalBounds();

    // Top Header (50px high)
    auto headerArea = area.removeFromTop(50).reduced(12, 8);
    titleLabel.setBounds(headerArea.removeFromLeft(120));

    // Duration preset buttons in header
    auto durGroup = headerArea.removeFromLeft(280).reduced(0, 3);
    int durBtnWidth = durGroup.getWidth() / 6;
    duration15sBtn.setBounds(durGroup.removeFromLeft(durBtnWidth).reduced(2, 0));
    duration30sBtn.setBounds(durGroup.removeFromLeft(durBtnWidth).reduced(2, 0));
    duration60sBtn.setBounds(durGroup.removeFromLeft(durBtnWidth).reduced(2, 0));
    duration120sBtn.setBounds(durGroup.removeFromLeft(durBtnWidth).reduced(2, 0));
    duration300sBtn.setBounds(durGroup.removeFromLeft(durBtnWidth).reduced(2, 0));
    duration600sBtn.setBounds(durGroup.reduced(2, 0));

    // Action buttons in header
    freezeBtn.setBounds(headerArea.removeFromLeft(75).reduced(4, 3));
    clearBtn.setBounds(headerArea.removeFromLeft(60).reduced(4, 3));
    auditionBtn.setBounds(headerArea.removeFromLeft(130).reduced(4, 3));
    normalizeBtn.setBounds(headerArea.removeFromLeft(70).reduced(4, 3));
    muteThruBtn.setBounds(headerArea.removeFromLeft(90).reduced(4, 3));

    // Bottom Bar (48px high)
    auto bottomArea = area.removeFromBottom(48).reduced(12, 8);
    uninstallBtn.setBounds(bottomArea.removeFromRight(85).reduced(2, 2));
    statusInfoLabel.setBounds(bottomArea.removeFromRight(170));

    dragAudioBtn.setBounds(bottomArea.removeFromRight(190).reduced(6, 2));
    timeSelectionLabel.setBounds(bottomArea);

    // Center Waveform Visualizer
    waveform.setBounds(area.reduced(10, 6));
}

} // namespace RecRoll
