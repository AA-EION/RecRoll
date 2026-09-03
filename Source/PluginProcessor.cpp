#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace RecRoll
{

juce::AudioProcessorValueTreeState::ParameterLayout RecRollAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID { "bufferDuration", 1 },
        "Buffer Duration",
        juce::StringArray { "15s", "30s", "1m", "2m", "5m", "10m" },
        2 // default 1m
    ));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "freezeBuffer", 1 },
        "Freeze Buffer",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "passthroughMuted", 1 },
        "Mute Passthrough",
        false
    ));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID { "normalizeExport", 1 },
        "Normalize Export",
        false
    ));

    return { params.begin(), params.end() };
}

RecRollAudioProcessor::RecRollAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "Parameters", createParameterLayout())
{
    bufferDurationParam   = apvts.getRawParameterValue("bufferDuration");
    freezeBufferParam     = apvts.getRawParameterValue("freezeBuffer");
    passthroughMutedParam = apvts.getRawParameterValue("passthroughMuted");
    normalizeParam        = apvts.getRawParameterValue("normalizeExport");

    // Pre-allocate rolling buffer for 10 minutes so buffer is never uninitialized
    rollingBuffer.prepare(44100.0, 2, RollingBuffer::DURATION_600S);
}

RecRollAudioProcessor::~RecRollAudioProcessor()
{
}

const juce::String RecRollAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool RecRollAudioProcessor::acceptsMidi() const { return false; }
bool RecRollAudioProcessor::producesMidi() const { return false; }
bool RecRollAudioProcessor::isMidiEffect() const { return false; }
double RecRollAudioProcessor::getTailLengthSeconds() const { return 0.0; }

int RecRollAudioProcessor::getNumPrograms() { return 1; }
int RecRollAudioProcessor::getCurrentProgram() { return 0; }
void RecRollAudioProcessor::setCurrentProgram(int) {}
const juce::String RecRollAudioProcessor::getProgramName(int) { return {}; }
void RecRollAudioProcessor::changeProgramName(int, const juce::String&) {}

void RecRollAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    setLatencySamples(0); // 100% zero latency

    rollingBuffer.prepare(sampleRate, getTotalNumInputChannels(), RollingBuffer::DURATION_600S);

    if (bufferDurationParam != nullptr)
    {
        int choice = static_cast<int>(bufferDurationParam->load());
        double seconds = 60.0;
        switch (choice)
        {
            case 0: seconds = RollingBuffer::DURATION_15S; break;
            case 1: seconds = RollingBuffer::DURATION_30S; break;
            case 2: seconds = RollingBuffer::DURATION_60S; break;
            case 3: seconds = RollingBuffer::DURATION_120S; break;
            case 4: seconds = RollingBuffer::DURATION_300S; break;
            case 5: seconds = RollingBuffer::DURATION_600S; break;
            default: seconds = 60.0; break;
        }
        rollingBuffer.setVisibleDurationSeconds(seconds);
    }
}

void RecRollAudioProcessor::releaseResources()
{
    rollingBuffer.stopAudition();
}

bool RecRollAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Stereo or Mono passthrough supported
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // Input layout must match output layout for transparent audio passthrough
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;

    return true;
}

void RecRollAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();

    // Clear unused output channels
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    // Update settings from parameters
    if (freezeBufferParam != nullptr)
        rollingBuffer.setRecording(freezeBufferParam->load() < 0.5f); // false = not frozen = recording active

    if (passthroughMutedParam != nullptr)
        rollingBuffer.setPassthroughMuted(passthroughMutedParam->load() > 0.5f);

    if (bufferDurationParam != nullptr)
    {
        int choice = static_cast<int>(bufferDurationParam->load());
        double seconds = 60.0;
        switch (choice)
        {
            case 0: seconds = RollingBuffer::DURATION_15S; break;
            case 1: seconds = RollingBuffer::DURATION_30S; break;
            case 2: seconds = RollingBuffer::DURATION_60S; break;
            case 3: seconds = RollingBuffer::DURATION_120S; break;
            case 4: seconds = RollingBuffer::DURATION_300S; break;
            case 5: seconds = RollingBuffer::DURATION_600S; break;
            default: seconds = 60.0; break;
        }
        rollingBuffer.setVisibleDurationSeconds(seconds);
    }

    // 1. Continuously record incoming audio stream into circular rolling buffer
    rollingBuffer.write(buffer);

    // 2. Passthrough logic:
    // If passthrough is muted, clear audio before audition mixing
    if (rollingBuffer.isPassthroughMuted())
    {
        buffer.clear();
    }

    // 3. Audition playback mixing:
    // If user is auditioning a slice within the plugin, mix into output
    if (rollingBuffer.isAuditioning())
    {
        rollingBuffer.processAuditionBlock(buffer);
    }
}

bool RecRollAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* RecRollAudioProcessor::createEditor()
{
    return new RecRollAudioProcessorEditor(*this);
}

void RecRollAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void RecRollAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
    {
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
    }
}

} // namespace RecRoll

// JUCE plugin entry point
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new RecRoll::RecRollAudioProcessor();
}
