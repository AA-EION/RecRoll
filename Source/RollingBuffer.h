#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <atomic>
#include <vector>
#include <memory>

namespace RecRoll
{

/**
 * Realtime-safe lock-free circular audio buffer.
 * Supports stereo audio recording with zero dynamic allocations in the audio thread.
 * Maintains a multi-scale peak overview cache for 60 FPS waveform rendering.
 */
class RollingBuffer
{
public:
    // Downsampling factor for peak overview cache (samples per peak bucket)
    static constexpr int SAMPLES_PER_PEAK = 256;

    // Supported buffer duration presets in seconds
    static constexpr double DURATION_15S  = 15.0;
    static constexpr double DURATION_30S  = 30.0;
    static constexpr double DURATION_60S  = 60.0;
    static constexpr double DURATION_120S = 120.0;
    static constexpr double DURATION_300S = 300.0;
    static constexpr double DURATION_600S = 600.0; // 10 minutes max

    struct PeakData
    {
        float minL { 0.0f };
        float maxL { 0.0f };
        float minR { 0.0f };
        float maxR { 0.0f };
    };

    RollingBuffer();
    ~RollingBuffer() = default;

    /** Pre-allocates buffer memory for up to maxDurationSeconds at given sample rate. */
    void prepare(double sampleRate, int numChannels, double maxDurationSeconds = DURATION_600S);

    /** Resets and clears the buffer contents. */
    void clear();

    /**
     * Writes incoming audio block into circular buffer.
     * Guaranteed realtime-safe: no allocations, no locks, no system calls.
     */
    void write(const juce::AudioBuffer<float>& inputBuffer);

    /** Total samples written since the last clear, as an absolute timeline position. */
    int64_t getAbsolutePosition() const noexcept { return totalSamplesWritten.load(std::memory_order_acquire); }

    /**
     * Reads a slice from the circular buffer into destBuffer.
     * startSampleOffsetFromNow: 0 is current write head, positive values go back in time.
     */
    int readSlice(juce::AudioBuffer<float>& destBuffer, int64_t startSampleOffsetFromNow, int numSamplesToRead) const;

    /**
     * Reads a slice from the circular buffer into destBuffer starting at an absolute timeline position.
     * absoluteStartSample: 0 is timeline start, up to totalSamplesWritten.
     */
    int readSliceAbsolute(juce::AudioBuffer<float>& destBuffer, int64_t absoluteStartSample, int numSamplesToRead) const;

    /** Returns current sample rate. */
    double getSampleRate() const noexcept { return currentSampleRate; }

    /** Returns total allocated capacity in audio samples. */
    int64_t getCapacitySamples() const noexcept { return bufferCapacitySamples; }

    /** Returns total recorded samples since clear/start. */
    int64_t getTotalSamplesWritten() const noexcept { return totalSamplesWritten.load(std::memory_order_acquire); }

    /** Returns current write head position in ring buffer. */
    int64_t getWriteHead() const noexcept { return writeHead.load(std::memory_order_acquire); }

    /** Returns active buffer duration in seconds (view range). */
    double getVisibleDurationSeconds() const noexcept { return visibleDurationSeconds.load(std::memory_order_relaxed); }
    void setVisibleDurationSeconds(double seconds);

    /** Recording active / freeze state. */
    bool isRecording() const noexcept { return recordingActive.load(std::memory_order_relaxed); }
    void setRecording(bool active) { recordingActive.store(active, std::memory_order_release); }

    /** Mute passthrough state. */
    bool isPassthroughMuted() const noexcept { return passthroughMuted.load(std::memory_order_relaxed); }
    void setPassthroughMuted(bool muted) { passthroughMuted.store(muted, std::memory_order_release); }

    /**
     * Peak Cache Access:
     * Fills an array of PeakData corresponding to the visible time window.
     * targetBucketCount: how many horizontal pixels/buckets to render.
     * endSamplePosition: absolute timeline sample for the right edge (-1 = live write head).
     */
    void getVisiblePeaks(std::vector<PeakData>& outPeaks, int targetBucketCount, int64_t endSamplePosition = -1) const;

    // --- Audition Playback Engine ---
    void startAudition(int64_t startSampleOffsetFromNow, int64_t numSamples);
    void startAuditionAbsolute(int64_t absoluteStartSample, int64_t numSamples);
    void stopAudition();
    bool isAuditioning() const noexcept { return auditionPlaying.load(std::memory_order_relaxed); }
    double getAuditionProgress() const;
    void processAuditionBlock(juce::AudioBuffer<float>& outputBuffer);

private:
    double currentSampleRate { 44100.0 };
    int numAudioChannels { 2 };
    int64_t bufferCapacitySamples { 0 };

    // Audio sample storage [channel][sample]
    std::vector<std::vector<float>> audioChannels;

    // Lock-free write markers
    std::atomic<int64_t> writeHead { 0 };
    std::atomic<int64_t> totalSamplesWritten { 0 };

    // Settings
    std::atomic<double> visibleDurationSeconds { DURATION_60S };
    std::atomic<bool> recordingActive { true };
    std::atomic<bool> passthroughMuted { false };

    // Peak overview cache
    int64_t peakCapacity { 0 };
    std::vector<PeakData> peakBuffer;
    std::atomic<int64_t> peakWriteHead { 0 };

    // Accumulator for building peak buckets on the audio thread
    int peakAccumulatorCount { 0 };
    PeakData currentBucketPeak;

    // Audition playback state.
    // auditionAbsoluteStart is a position on the absolute recorded timeline, not
    // a distance back from the write head. A head-relative start would drift:
    // the head advances one block per callback and the play cursor advances
    // another, so playback ran at double speed whenever recording was live.
    std::atomic<bool> auditionPlaying { false };
    std::atomic<int64_t> auditionAbsoluteStart { 0 };
    std::atomic<int64_t> auditionLengthSamples { 0 };
    std::atomic<int64_t> auditionCurrentOffset { 0 };
};

} // namespace RecRoll
