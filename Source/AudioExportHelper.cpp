#include "AudioExportHelper.h"
#include <algorithm>
#include <cmath>

namespace RecRoll
{

juce::File AudioExportHelper::getExportDirectory()
{
    auto tempDir = juce::File::getSpecialLocation(juce::File::tempDirectory);
    auto exportDir = tempDir.getChildFile("RecRoll_Exports");

    if (!exportDir.exists())
        exportDir.createDirectory();

    return exportDir;
}

void AudioExportHelper::cleanupOldTempFiles()
{
    auto exportDir = getExportDirectory();
    if (!exportDir.isDirectory())
        return;

    auto now = juce::Time::getCurrentTime();
    auto files = exportDir.findChildFiles(juce::File::findFiles, false, "RecRoll_*.wav");

    for (const auto& file : files)
    {
        // Delete files older than 2 hours to avoid disk clutter
        if ((now - file.getLastModificationTime()).inHours() >= 2)
        {
            file.deleteFile();
        }
    }
}

juce::File AudioExportHelper::exportSliceToTempWav(const RollingBuffer& buffer,
                                                  int64_t startOffsetFromNow,
                                                  int64_t numSamples,
                                                  bool normalize)
{
    cleanupOldTempFiles();

    auto exportDir = getExportDirectory();
    auto timestamp = juce::Time::getCurrentTime().formatted("%Y-%m-%d_%H-%M-%S");
    auto targetFile = exportDir.getChildFile("RecRoll_" + timestamp + ".wav");

    // Avoid collision if multiple drags happen in the same second
    int counter = 1;
    while (targetFile.existsAsFile())
    {
        targetFile = exportDir.getChildFile("RecRoll_" + timestamp + "_" + juce::String(counter++) + ".wav");
    }

    if (numSamples <= 0)
        numSamples = static_cast<int64_t>(buffer.getSampleRate() * 1.0); // 1 second fallback

    juce::AudioBuffer<float> slice(2, static_cast<int>(numSamples));
    int samplesRead = const_cast<RollingBuffer&>(buffer).readSlice(slice, startOffsetFromNow, static_cast<int>(numSamples));

    if (samplesRead <= 0)
    {
        slice.clear();
        samplesRead = static_cast<int>(numSamples);
    }

    // Apply normalization if enabled
    if (normalize && samplesRead > 0)
    {
        float maxPeak = 0.0f;
        for (int ch = 0; ch < slice.getNumChannels(); ++ch)
        {
            maxPeak = std::max(maxPeak, slice.getMagnitude(ch, 0, samplesRead));
        }

        if (maxPeak > 0.0001f)
        {
            // Target peak: -0.1 dBFS (~0.9885)
            float targetGain = 0.9885f / maxPeak;
            slice.applyGain(0, samplesRead, targetGain);
        }
    }

    // Write standard 24-bit PCM WAV
    juce::WavAudioFormat wavFormat;
    std::unique_ptr<juce::FileOutputStream> outStream(targetFile.createOutputStream());

    if (outStream != nullptr)
    {
        std::unique_ptr<juce::AudioFormatWriter> writer(
            wavFormat.createWriterFor(outStream.get(),
                                     buffer.getSampleRate(),
                                     static_cast<unsigned int>(slice.getNumChannels()),
                                     24, // 24-bit PCM
                                     {},
                                     0));

        if (writer != nullptr)
        {
            outStream.release(); // AudioFormatWriter takes ownership
            writer->writeFromAudioSampleBuffer(slice, 0, samplesRead);
            writer.reset();
        }
    }

    return targetFile;
}

} // namespace RecRoll
