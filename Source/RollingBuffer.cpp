#include "RollingBuffer.h"
#include <algorithm>
#include <cmath>

namespace RecRoll
{

RollingBuffer::RollingBuffer()
{
    currentBucketPeak = { 0.0f, 0.0f, 0.0f, 0.0f };
}

void RollingBuffer::prepare(double sampleRate, int numChannels, double maxDurationSeconds)
{
    double targetRate = (sampleRate > 0.0) ? sampleRate : 44100.0;
    int targetChannels = std::max(2, numChannels);

    // If already prepared at this sample rate with full capacity, preserve existing audio history!
    if (std::abs(currentSampleRate - targetRate) < 1.0
        && bufferCapacitySamples > 0
        && static_cast<int>(audioChannels.size()) >= targetChannels)
    {
        return;
    }

    currentSampleRate = targetRate;
    numAudioChannels = targetChannels;

    // Round up capacity
    bufferCapacitySamples = static_cast<int64_t>(std::ceil(currentSampleRate * maxDurationSeconds));
    if (bufferCapacitySamples < 1024)
        bufferCapacitySamples = 1024;

    audioChannels.resize(static_cast<size_t>(numAudioChannels));
    for (auto& ch : audioChannels)
    {
        ch.assign(static_cast<size_t>(bufferCapacitySamples), 0.0f);
    }

    // Peak overview cache capacity (1 peak entry per SAMPLES_PER_PEAK audio frames)
    peakCapacity = (bufferCapacitySamples / SAMPLES_PER_PEAK) + 16;
    peakBuffer.assign(static_cast<size_t>(peakCapacity), PeakData{});

    clear();
}

void RollingBuffer::clear()
{
    stopAudition();

    writeHead.store(0, std::memory_order_release);
    totalSamplesWritten.store(0, std::memory_order_release);
    peakWriteHead.store(0, std::memory_order_release);

    for (auto& ch : audioChannels)
        std::fill(ch.begin(), ch.end(), 0.0f);

    std::fill(peakBuffer.begin(), peakBuffer.end(), PeakData{});

    peakAccumulatorCount = 0;
    currentBucketPeak = { 0.0f, 0.0f, 0.0f, 0.0f };
}

void RollingBuffer::setVisibleDurationSeconds(double seconds)
{
    double clamped = std::clamp(seconds, 5.0, DURATION_600S);
    visibleDurationSeconds.store(clamped, std::memory_order_relaxed);
}

void RollingBuffer::write(const juce::AudioBuffer<float>& inputBuffer)
{
    const int numSamples = inputBuffer.getNumSamples();
    const int inputChannels = inputBuffer.getNumChannels();
    if (numSamples <= 0 || inputChannels <= 0 || bufferCapacitySamples <= 0)
        return;

    if (!recordingActive.load(std::memory_order_relaxed))
        return;

    int64_t currentHead = writeHead.load(std::memory_order_relaxed);
    int64_t currentPeakHead = peakWriteHead.load(std::memory_order_relaxed);

    const float* inL = inputBuffer.getReadPointer(0);
    const float* inR = (inputChannels > 1) ? inputBuffer.getReadPointer(1) : inL;

    for (int i = 0; i < numSamples; ++i)
    {
        const float sL = inL[i];
        const float sR = inR[i];

        // Store into circular audio buffer
        audioChannels[0][static_cast<size_t>(currentHead)] = sL;
        if (numAudioChannels > 1)
            audioChannels[1][static_cast<size_t>(currentHead)] = sR;

        // Accumulate peak values
        if (peakAccumulatorCount == 0)
        {
            currentBucketPeak.minL = sL;
            currentBucketPeak.maxL = sL;
            currentBucketPeak.minR = sR;
            currentBucketPeak.maxR = sR;
        }
        else
        {
            currentBucketPeak.minL = std::min(currentBucketPeak.minL, sL);
            currentBucketPeak.maxL = std::max(currentBucketPeak.maxL, sL);
            currentBucketPeak.minR = std::min(currentBucketPeak.minR, sR);
            currentBucketPeak.maxR = std::max(currentBucketPeak.maxR, sR);
        }

        peakAccumulatorCount++;
        if (peakAccumulatorCount >= SAMPLES_PER_PEAK)
        {
            peakBuffer[static_cast<size_t>(currentPeakHead)] = currentBucketPeak;
            currentPeakHead = (currentPeakHead + 1) % peakCapacity;
            peakAccumulatorCount = 0;
            currentBucketPeak = { 0.0f, 0.0f, 0.0f, 0.0f };
        }

        currentHead = (currentHead + 1) % bufferCapacitySamples;
    }

    writeHead.store(currentHead, std::memory_order_release);
    peakWriteHead.store(currentPeakHead, std::memory_order_release);
    totalSamplesWritten.fetch_add(numSamples, std::memory_order_release);
}

int RollingBuffer::readSlice(juce::AudioBuffer<float>& destBuffer, int64_t startSampleOffsetFromNow, int numSamplesToRead) const
{
    if (bufferCapacitySamples <= 0 || numSamplesToRead <= 0)
        return 0;

    const int64_t head = writeHead.load(std::memory_order_acquire);
    const int64_t totalWritten = totalSamplesWritten.load(std::memory_order_acquire);

    // Limit offset to available history
    int64_t maxAvailable = std::min(totalWritten, bufferCapacitySamples);
    if (startSampleOffsetFromNow > maxAvailable)
        startSampleOffsetFromNow = maxAvailable;

    if (numSamplesToRead > startSampleOffsetFromNow)
        numSamplesToRead = static_cast<int>(startSampleOffsetFromNow);

    if (numSamplesToRead <= 0)
        return 0;

    // The oldest sample of the slice in circular buffer
    int64_t readStart = head - startSampleOffsetFromNow;
    while (readStart < 0)
        readStart += bufferCapacitySamples;

    const int destChannels = std::min(destBuffer.getNumChannels(), numAudioChannels);
    destBuffer.setSize(destChannels, numSamplesToRead, false, false, true);

    for (int ch = 0; ch < destChannels; ++ch)
    {
        float* dest = destBuffer.getWritePointer(ch);
        const auto& src = audioChannels[static_cast<size_t>(ch)];

        int64_t firstChunk = std::min(static_cast<int64_t>(numSamplesToRead), bufferCapacitySamples - readStart);
        std::copy(src.begin() + readStart, src.begin() + readStart + firstChunk, dest);

        if (numSamplesToRead > firstChunk)
        {
            int64_t secondChunk = numSamplesToRead - firstChunk;
            std::copy(src.begin(), src.begin() + secondChunk, dest + firstChunk);
        }
    }

    return numSamplesToRead;
}

void RollingBuffer::getVisiblePeaks(std::vector<PeakData>& outPeaks, int targetBucketCount) const
{
    if (targetBucketCount <= 0 || peakCapacity <= 0)
        return;

    outPeaks.assign(static_cast<size_t>(targetBucketCount), PeakData{ 0.0f, 0.0f, 0.0f, 0.0f });

    const double visSec = visibleDurationSeconds.load(std::memory_order_relaxed);
    const int64_t totalVisibleSamples = static_cast<int64_t>(currentSampleRate * visSec);
    const int64_t totalVisiblePeaks = totalVisibleSamples / SAMPLES_PER_PEAK;

    if (totalVisiblePeaks <= 0)
        return;

    const int64_t curPeakHead = peakWriteHead.load(std::memory_order_acquire);
    const int64_t totalWritten = totalSamplesWritten.load(std::memory_order_acquire);
    const int64_t availablePeaks = std::min(totalWritten / SAMPLES_PER_PEAK, peakCapacity);

    // Oldest visible peak position
    const int64_t peaksToDisplay = std::min(totalVisiblePeaks, peakCapacity);

    for (int bucketIdx = 0; bucketIdx < targetBucketCount; ++bucketIdx)
    {
        // Calculate the slice of peaks that map to this display bucket
        double t0 = static_cast<double>(bucketIdx) / targetBucketCount;
        double t1 = static_cast<double>(bucketIdx + 1) / targetBucketCount;

        // t0 = 0 is oldest (left), t0 = 1 is newest (right, now)
        int64_t peakOffsetStart = static_cast<int64_t>((1.0 - t0) * peaksToDisplay);
        int64_t peakOffsetEnd   = static_cast<int64_t>((1.0 - t1) * peaksToDisplay);

        if (peakOffsetStart < peakOffsetEnd)
            std::swap(peakOffsetStart, peakOffsetEnd);

        PeakData bucketPeak { 0.0f, 0.0f, 0.0f, 0.0f };
        bool first = true;

        for (int64_t offset = peakOffsetStart; offset >= peakOffsetEnd; --offset)
        {
            if (offset >= availablePeaks)
                continue;

            int64_t idx = (curPeakHead - 1) - offset;
            while (idx < 0) idx += peakCapacity;
            idx %= peakCapacity;

            const auto& p = peakBuffer[static_cast<size_t>(idx)];
            if (first)
            {
                bucketPeak = p;
                first = false;
            }
            else
            {
                bucketPeak.minL = std::min(bucketPeak.minL, p.minL);
                bucketPeak.maxL = std::max(bucketPeak.maxL, p.maxL);
                bucketPeak.minR = std::min(bucketPeak.minR, p.minR);
                bucketPeak.maxR = std::max(bucketPeak.maxR, p.maxR);
            }
        }

        outPeaks[static_cast<size_t>(bucketIdx)] = bucketPeak;
    }

    // If audio is actively arriving but hasn't completed a full SAMPLES_PER_PEAK bucket yet,
    // ensure the newest pixel bucket immediately reflects the current live audio peak!
    if (availablePeaks == 0 && totalWritten > 0 && targetBucketCount > 0)
    {
        outPeaks.back() = currentBucketPeak;
    }
}

// --- Audition Playback Engine ---

void RollingBuffer::startAudition(int64_t startSampleOffsetFromNow, int64_t numSamples)
{
    if (numSamples <= 0)
        return;

    // Pin the selection to the absolute timeline once, here. Everything after
    // this reads from a fixed point rather than chasing a moving write head.
    const int64_t absoluteStart = totalSamplesWritten.load(std::memory_order_acquire)
                                - startSampleOffsetFromNow;

    auditionAbsoluteStart.store(absoluteStart, std::memory_order_relaxed);
    auditionLengthSamples.store(numSamples, std::memory_order_relaxed);
    auditionCurrentOffset.store(0, std::memory_order_relaxed);
    auditionPlaying.store(true, std::memory_order_release);
}

void RollingBuffer::stopAudition()
{
    auditionPlaying.store(false, std::memory_order_release);
    auditionCurrentOffset.store(0, std::memory_order_relaxed);
}

double RollingBuffer::getAuditionProgress() const
{
    if (!auditionPlaying.load(std::memory_order_relaxed))
        return 0.0;

    int64_t total = auditionLengthSamples.load(std::memory_order_relaxed);
    if (total <= 0)
        return 0.0;

    int64_t current = auditionCurrentOffset.load(std::memory_order_relaxed);
    return std::clamp(static_cast<double>(current) / static_cast<double>(total), 0.0, 1.0);
}

void RollingBuffer::processAuditionBlock(juce::AudioBuffer<float>& outputBuffer)
{
    if (!auditionPlaying.load(std::memory_order_relaxed))
        return;

    const int numSamples = outputBuffer.getNumSamples();
    const int numChannels = std::min(outputBuffer.getNumChannels(), numAudioChannels);

    const int64_t absoluteStart = auditionAbsoluteStart.load(std::memory_order_relaxed);
    const int64_t totalAuditionLen = auditionLengthSamples.load(std::memory_order_relaxed);
    int64_t curOffset = auditionCurrentOffset.load(std::memory_order_relaxed);

    if (curOffset >= totalAuditionLen)
    {
        stopAudition();
        return;
    }

    const int samplesToPlay = static_cast<int>(std::min(static_cast<int64_t>(numSamples), totalAuditionLen - curOffset));

    // Map the absolute timeline position onto the ring. The play cursor now
    // advances only by what we consume, independently of the write head.
    int64_t sourcePos = (absoluteStart + curOffset) % bufferCapacitySamples;
    while (sourcePos < 0) sourcePos += bufferCapacitySamples;

    for (int ch = 0; ch < numChannels; ++ch)
    {
        float* out = outputBuffer.getWritePointer(ch);
        const auto& src = audioChannels[static_cast<size_t>(ch)];

        for (int i = 0; i < samplesToPlay; ++i)
        {
            int64_t idx = (sourcePos + i) % bufferCapacitySamples;
            // Mix audition audio into output
            out[i] += src[static_cast<size_t>(idx)];
        }
    }

    curOffset += samplesToPlay;
    auditionCurrentOffset.store(curOffset, std::memory_order_relaxed);

    if (curOffset >= totalAuditionLen)
    {
        stopAudition();
    }
}

} // namespace RecRoll
