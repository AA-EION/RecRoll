#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "RollingBuffer.h"

#if __has_include("clap-juce-extensions/clap-juce-extensions.h")
#include "clap-juce-extensions/clap-juce-extensions.h"
#define RECROLL_HAS_CLAP 1
#else
#define RECROLL_HAS_CLAP 0
#endif

namespace RecRoll
{

class RecRollAudioProcessor : public juce::AudioProcessor
#if RECROLL_HAS_CLAP
                            , public clap_juce_extensions::clap_properties
#endif
{
public:
    RecRollAudioProcessor();
    ~RecRollAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    RollingBuffer& getRollingBuffer() noexcept { return rollingBuffer; }

    juce::AudioProcessorValueTreeState& getAPVTS() noexcept { return apvts; }

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

private:
    RollingBuffer rollingBuffer;
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float>* bufferDurationParam { nullptr };
    std::atomic<float>* freezeBufferParam { nullptr };
    std::atomic<float>* passthroughMutedParam { nullptr };
    std::atomic<float>* normalizeParam { nullptr };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(RecRollAudioProcessor)
};

} // namespace RecRoll
