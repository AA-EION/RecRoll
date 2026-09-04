#pragma once

#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_core/juce_core.h>
#include "RollingBuffer.h"

namespace RecRoll
{

class AudioExportHelper
{
public:
    /**
     * Extracts the requested time slice from the rolling buffer and exports it to a temporary WAV file.
     * @param buffer The source RollingBuffer.
     * @param startOffsetFromNow How many samples back in time the slice begins.
     * @param numSamples Length of slice in audio samples.
     * @param normalize If true, applies peak normalization to -0.1 dBFS.
     * @return The temporary juce::File pointing to the newly rendered .wav file.
     */
    static juce::File exportSliceToTempWav(const RollingBuffer& buffer,
                                          int64_t startOffsetFromNow,
                                          int64_t numSamples,
                                          bool normalize);

    /**
     * Extracts the requested time slice starting at an absolute sample timeline position and exports to WAV.
     * @param buffer The source RollingBuffer.
     * @param absoluteStartSample Absolute sample timeline position where slice begins.
     * @param numSamples Length of slice in audio samples.
     * @param normalize If true, applies peak normalization to -0.1 dBFS.
     * @return The temporary juce::File pointing to the newly rendered .wav file.
     */
    static juce::File exportAbsoluteSliceToTempWav(const RollingBuffer& buffer,
                                                  int64_t absoluteStartSample,
                                                  int64_t numSamples,
                                                  bool normalize);

    /** Returns the export directory inside the system temp folder. */
    static juce::File getExportDirectory();

    /** Cleans up temporary exported WAV files older than 2 hours. */
    static void cleanupOldTempFiles();
};

} // namespace RecRoll
