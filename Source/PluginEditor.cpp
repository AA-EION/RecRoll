#include "PluginEditor.h"
#include "UninstallerHelper.h"
#include <iomanip>
#include <sstream>

namespace RecRoll
{

RecRollAudioProcessorEditor::DonationBanner::DonationBanner()
{
    messageLabel.setText("Support RecRoll open-source development! ❤️", juce::dontSendNotification);
    messageLabel.setFont(juce::FontOptions(13.0f));
    messageLabel.setColour(juce::Label::textColourId, juce::Colour(0xffffe082));
    addAndMakeVisible(messageLabel);

    donateBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xffffb300));
    donateBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff1a1300));
    addAndMakeVisible(donateBtn);

    dismissBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0x00000000));
    dismissBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0bec5));
    addAndMakeVisible(dismissBtn);
}

void RecRollAudioProcessorEditor::DonationBanner::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff292015)); // Warm amber banner
    g.setColour(juce::Colour(0x66ffb300));
    g.drawRect(getLocalBounds(), 1);
}

void RecRollAudioProcessorEditor::DonationBanner::resized()
{
    auto b = getLocalBounds().reduced(8, 4);
    dismissBtn.setBounds(b.removeFromRight(28));
    donateBtn.setBounds(b.removeFromRight(150).reduced(4, 1));
    messageLabel.setBounds(b);
}

RecRollAudioProcessorEditor::RecRollAudioProcessorEditor(RecRollAudioProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p), waveform(p.getRollingBuffer())
{
    // Make window nicely resizable
    setResizable(true, true);
    setResizeLimits(700, 380, 1920, 1200);

    // Title Label
    titleLabel.setText("RECROLL", juce::dontSendNotification);
    titleLabel.setFont(juce::FontOptions(20.0f).withKerningFactor(0.12f));
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

    // Persistent Donate button in header
    donateHeaderBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff332612));
    donateHeaderBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffffc107));
    donateHeaderBtn.setTooltip("Support RecRoll development via PayPal");
    donateHeaderBtn.addListener(this);
    addAndMakeVisible(donateHeaderBtn);

    // Session Donation Banner (visible on every session load)
    donationBanner.donateBtn.addListener(this);
    donationBanner.dismissBtn.addListener(this);
    addAndMakeVisible(donationBanner);

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
    dragAudioBtn.addMouseListener(&dragButtonListener, false);
    dragAudioBtn.onClick = [this]()
    {
        // A drag already handed to the OS has done the export; a plain click
        // still exports the current selection.
        if (!dragButtonListener.startedDrag)
            waveform.startDawDragOperation();
    };
    addAndMakeVisible(dragAudioBtn);

    juce::String srStr = juce::String(processorRef.getSampleRate() / 1000.0, 1) + " kHz";
    statusInfoLabel.setText(srStr + " | Clean Passthrough", juce::dontSendNotification);
    statusInfoLabel.setFont(juce::FontOptions(11.5f));
    statusInfoLabel.setColour(juce::Label::textColourId, juce::Colour(0xff606770));
    statusInfoLabel.setJustificationType(juce::Justification::centredRight);
    addAndMakeVisible(statusInfoLabel);

    // About button - opens the overlay describing who makes RecRoll and how to
    // support it. Lives in the bottom bar, where there is slack at every window
    // size, rather than in the already-crowded header.
    aboutBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2029));
    aboutBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xffb0b8c4));
    aboutBtn.setTooltip("About RecRoll - EION Studios, credits, licence and support");
    aboutBtn.addListener(this);
    addAndMakeVisible(aboutBtn);

    aboutPanel.onDismiss = [this]
    {
        aboutPanel.setVisible(false);
        grabKeyboardFocus();
    };
    aboutPanel.onDonate = [this] { openDonationLink(); };
    addChildComponent(aboutPanel);

    // Uninstall button - ONLY shown on Standalone application, hidden in VST3, CLAP, and AU
    bool isStandalone = (processorRef.wrapperType == juce::AudioProcessor::wrapperType_Standalone)
                     || juce::JUCEApplicationBase::isStandaloneApp();
    uninstallBtn.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff1e2029));
    uninstallBtn.setColour(juce::TextButton::textColourOffId, juce::Colour(0xff888888));
    uninstallBtn.setTooltip("Completely uninstall RecRoll plugins and application from system");
    uninstallBtn.addListener(this);
    uninstallBtn.setVisible(isStandalone);
    if (isStandalone)
        addAndMakeVisible(uninstallBtn);

    // Connect APVTS attachments
    freezeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "freezeBuffer", freezeBtn);

    muteThruAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "passthroughMuted", muteThruBtn);

    normalizeAttachment = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment>(
        processorRef.getAPVTS(), "normalizeExport", normalizeBtn);

    updateDurationButtonStyles(processorRef.getRollingBuffer().getVisibleDurationSeconds());

    // Set initial size after all children are added and configured so resized() lays out everything
    setSize(860, 480);
}

RecRollAudioProcessorEditor::~RecRollAudioProcessorEditor()
{
    dragAudioBtn.removeMouseListener(&dragButtonListener);
    donateHeaderBtn.removeListener(this);
    aboutBtn.removeListener(this);
    donationBanner.donateBtn.removeListener(this);
    donationBanner.dismissBtn.removeListener(this);
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

void RecRollAudioProcessorEditor::openDonationLink()
{
    juce::URL("https://www.paypal.com/donate/?business=juanesgtgt2@gmail.com&no_recurring=0&item_name=RecRoll+Plugin+Support&currency_code=USD").launchInDefaultBrowser();
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
    else if (button == &donateHeaderBtn || button == &donationBanner.donateBtn)
    {
        openDonationLink();
    }
    else if (button == &donationBanner.dismissBtn)
    {
        donationBanner.setVisible(false);
        resized();
    }
    else if (button == &aboutBtn)
    {
        aboutPanel.show();
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

    donateHeaderBtn.setBounds(headerArea.removeFromRight(80).reduced(2, 2));

    // Session Donation Banner (Appears each time editor is loaded into a session)
    if (donationBanner.isVisible())
    {
        donationBanner.setBounds(area.removeFromTop(36).reduced(10, 2));
    }

    // Bottom Bar (48px high)
    auto bottomArea = area.removeFromBottom(48).reduced(12, 8);
    if (uninstallBtn.isVisible())
        uninstallBtn.setBounds(bottomArea.removeFromRight(85).reduced(2, 2));
    aboutBtn.setBounds(bottomArea.removeFromRight(66).reduced(2, 2));
    statusInfoLabel.setBounds(bottomArea.removeFromRight(170));

    dragAudioBtn.setBounds(bottomArea.removeFromRight(190).reduced(6, 2));
    timeSelectionLabel.setBounds(bottomArea);

    // Center Waveform Visualizer
    waveform.setBounds(area.reduced(10, 6));

    // The About overlay covers the whole editor while it is open.
    aboutPanel.setBounds(getLocalBounds());
}

} // namespace RecRoll
