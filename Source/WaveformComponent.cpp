#include "WaveformComponent.h"
#include "AudioExportHelper.h"
#include <iomanip>
#include <sstream>

namespace RecRoll
{

WaveformComponent::WaveformComponent(RollingBuffer& b)
    : buffer(b)
{
    setOpaque(true);
    startTimerHz(60); // Smooth 60 FPS UI refresh
}

WaveformComponent::~WaveformComponent()
{
    stopTimer();
}

void WaveformComponent::resized()
{
    auto b = getLocalBounds().toFloat();
    timelineBounds = b.removeFromBottom(22.0f);
    waveformBounds = b.reduced(4.0f, 4.0f);
}

void WaveformComponent::timerCallback()
{
    repaint();
}

double WaveformComponent::xToNormalized(float x) const
{
    if (waveformBounds.getWidth() <= 0.0f)
        return 0.0;

    double norm = (x - waveformBounds.getX()) / waveformBounds.getWidth();
    return std::clamp(norm, 0.0, 1.0);
}

float WaveformComponent::normalizedToX(double norm) const
{
    return waveformBounds.getX() + static_cast<float>(norm * waveformBounds.getWidth());
}

void WaveformComponent::selectAll()
{
    selectionStartNormalized = 0.0;
    selectionEndNormalized = 1.0;
    hasCustomSelection = false;
    updateSelectionReadout();
    repaint();
}

void WaveformComponent::clearSelection()
{
    selectAll();
}

void WaveformComponent::updateSelectionReadout()
{
    if (!onSelectionChanged)
        return;

    const double visSec = buffer.getVisibleDurationSeconds();
    // 0.0 is oldest (left, -visSec), 1.0 is newest (right, 0s)
    double startSec = (selectionStartNormalized - 1.0) * visSec;
    double endSec   = (selectionEndNormalized - 1.0) * visSec;
    double durationSec = (selectionEndNormalized - selectionStartNormalized) * visSec;

    onSelectionChanged(startSec, endSec, durationSec);
}

void WaveformComponent::toggleAudition()
{
    if (buffer.isAuditioning())
    {
        buffer.stopAudition();
        repaint();
        return;
    }

    const double visSec = buffer.getVisibleDurationSeconds();
    const double sr = buffer.getSampleRate();

    // Calculate sample offsets
    // Start of selection is at selectionStartNormalized
    // Distance from write head (Now / 1.0) is (1.0 - selectionStartNormalized) * visSec
    double startOffsetSec = (1.0 - selectionStartNormalized) * visSec;
    double durationSec    = (selectionEndNormalized - selectionStartNormalized) * visSec;

    int64_t startSampleOffset = static_cast<int64_t>(startOffsetSec * sr);
    int64_t numSamples        = static_cast<int64_t>(durationSec * sr);

    if (numSamples > 100)
    {
        buffer.startAudition(startSampleOffset, numSamples);
    }
}

void WaveformComponent::startDawDragOperation()
{
    const double visSec = buffer.getVisibleDurationSeconds();
    const double sr = buffer.getSampleRate();

    double startOffsetSec = (1.0 - selectionStartNormalized) * visSec;
    double durationSec    = (selectionEndNormalized - selectionStartNormalized) * visSec;

    int64_t startSampleOffset = static_cast<int64_t>(startOffsetSec * sr);
    int64_t numSamples        = static_cast<int64_t>(durationSec * sr);

    if (numSamples <= 0)
        numSamples = static_cast<int64_t>(sr * 1.0);

    auto tempWav = AudioExportHelper::exportSliceToTempWav(buffer, startSampleOffset, numSamples, normalizeExport);

    if (tempWav.existsAsFile())
    {
        juce::StringArray files;
        files.add(tempWav.getFullPathName());
        performExternalDragDropOfFiles(files, false, this);
    }
}

void WaveformComponent::mouseDown(const juce::MouseEvent& e)
{
    dragStartPos = e.getPosition();
    hasInitiatedExternalDrag = false;

    float mouseX = static_cast<float>(e.x);
    float selStartX = normalizedToX(selectionStartNormalized);
    float selEndX   = normalizedToX(selectionEndNormalized);

    constexpr float HANDLE_GRAB_PX = 10.0f;

    // Check if clicking near selection handles
    if (std::abs(mouseX - selStartX) <= HANDLE_GRAB_PX)
    {
        currentDragMode = DragMode::MovingStartHandle;
    }
    else if (std::abs(mouseX - selEndX) <= HANDLE_GRAB_PX)
    {
        currentDragMode = DragMode::MovingEndHandle;
    }
    else if (mouseX > selStartX + HANDLE_GRAB_PX && mouseX < selEndX - HANDLE_GRAB_PX && hasCustomSelection)
    {
        // Inside existing selection: prepare for dragging into DAW
        currentDragMode = DragMode::ExportingToDaw;
    }
    else
    {
        // Start creating a new selection
        currentDragMode = DragMode::CreatingSelection;
        double norm = xToNormalized(mouseX);
        selectionStartNormalized = norm;
        selectionEndNormalized = norm;
        hasCustomSelection = true;
    }

    updateSelectionReadout();
    repaint();
}

void WaveformComponent::mouseDrag(const juce::MouseEvent& e)
{
    float mouseX = static_cast<float>(e.x);
    double normX = xToNormalized(mouseX);

    if (currentDragMode == DragMode::ExportingToDaw)
    {
        // If moved more than 8 pixels, trigger DAW drag
        if (!hasInitiatedExternalDrag && e.getDistanceFromDragStart() > 8)
        {
            hasInitiatedExternalDrag = true;
            startDawDragOperation();
        }
        return;
    }

    if (currentDragMode == DragMode::CreatingSelection)
    {
        double startNorm = xToNormalized(static_cast<float>(dragStartPos.x));
        selectionStartNormalized = std::min(startNorm, normX);
        selectionEndNormalized   = std::max(startNorm, normX);
        hasCustomSelection = true;
    }
    else if (currentDragMode == DragMode::MovingStartHandle)
    {
        selectionStartNormalized = std::min(normX, selectionEndNormalized - 0.005);
    }
    else if (currentDragMode == DragMode::MovingEndHandle)
    {
        selectionEndNormalized = std::max(normX, selectionStartNormalized + 0.005);
    }

    updateSelectionReadout();
    repaint();
}

void WaveformComponent::mouseUp(const juce::MouseEvent&)
{
    // If a tiny selection was made, expand to default or full
    if (hasCustomSelection && (selectionEndNormalized - selectionStartNormalized < 0.005))
    {
        selectAll();
    }

    currentDragMode = DragMode::None;
    hasInitiatedExternalDrag = false;
    updateSelectionReadout();
    repaint();
}

void WaveformComponent::mouseDoubleClick(const juce::MouseEvent&)
{
    selectAll();
}

void WaveformComponent::paint(juce::Graphics& g)
{
    // Dark background
    g.fillAll(juce::Colour(0xff13141a));

    const auto bounds = waveformBounds;
    if (bounds.getWidth() <= 10.0f || bounds.getHeight() <= 10.0f)
        return;

    // Center divider & grid background
    g.setColour(juce::Colour(0xff1b1d26));
    g.fillRect(bounds);

    const float centerY = bounds.getCentreY();
    const float halfHeight = (bounds.getHeight() - 4.0f) * 0.5f;

    // Time Grid lines
    const double visSec = buffer.getVisibleDurationSeconds();
    double gridIntervalSec = 10.0;
    if (visSec <= 15.0) gridIntervalSec = 2.0;
    else if (visSec <= 30.0) gridIntervalSec = 5.0;
    else if (visSec <= 60.0) gridIntervalSec = 10.0;
    else if (visSec <= 120.0) gridIntervalSec = 20.0;
    else if (visSec <= 300.0) gridIntervalSec = 60.0;
    else gridIntervalSec = 120.0;

    g.setColour(juce::Colour(0x22ffffff));
    for (double sec = gridIntervalSec; sec < visSec; sec += gridIntervalSec)
    {
        float norm = static_cast<float>(1.0 - (sec / visSec));
        float gx = normalizedToX(norm);
        g.drawVerticalLine(static_cast<int>(gx), bounds.getY(), bounds.getBottom());
    }

    // Waveform rendering from Peak cache
    const int pixelWidth = static_cast<int>(bounds.getWidth());
    bool hasAudioData = false;

    if (pixelWidth > 0)
    {
        buffer.getVisiblePeaks(peakCache, pixelWidth);

        // 1. Draw glowing background filled envelope
        g.setColour(juce::Colour(0x3300e5ff));
        for (int x = 0; x < pixelWidth && x < static_cast<int>(peakCache.size()); ++x)
        {
            float px = bounds.getX() + static_cast<float>(x);
            const auto& p = peakCache[static_cast<size_t>(x)];

            float maxMag = std::max({ std::abs(p.minL), std::abs(p.maxL), std::abs(p.minR), std::abs(p.maxR) });

            if (maxMag > 0.0001f)
            {
                hasAudioData = true;
                float peakPositive = std::max({ 0.0f, p.maxL, p.maxR });
                float peakNegative = std::min({ 0.0f, p.minL, p.minR });

                float yTop = centerY - (peakPositive * halfHeight * 0.95f);
                float yBottom = centerY - (peakNegative * halfHeight * 0.95f);
                float height = std::max(2.0f, yBottom - yTop);

                g.fillRect(juce::Rectangle<float>(px, yTop, 1.0f, height));
            }
        }

        // 2. Draw solid bright electric cyan waveform bars
        g.setColour(juce::Colour(0xff00e5ff));
        for (int x = 0; x < pixelWidth && x < static_cast<int>(peakCache.size()); ++x)
        {
            float px = bounds.getX() + static_cast<float>(x);
            const auto& p = peakCache[static_cast<size_t>(x)];

            float maxMag = std::max({ std::abs(p.minL), std::abs(p.maxL), std::abs(p.minR), std::abs(p.maxR) });

            if (maxMag > 0.0001f)
            {
                float peakPositive = std::max({ 0.0f, p.maxL, p.maxR });
                float peakNegative = std::min({ 0.0f, p.minL, p.minR });

                float yTop = centerY - (peakPositive * halfHeight * 0.95f);
                float yBottom = centerY - (peakNegative * halfHeight * 0.95f);
                float height = std::max(2.0f, yBottom - yTop);

                g.fillRect(juce::Rectangle<float>(px, yTop, 1.0f, height));
            }
        }
    }

    // If no audio has been captured yet, show a subtle guiding message in the center
    if (!hasAudioData && buffer.getTotalSamplesWritten() == 0)
    {
        g.setColour(juce::Colour(0x66ffffff));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText("Waiting for incoming audio... (Play audio in DAW track or feed sound into input)",
                   bounds, juce::Justification::centred, false);
    }

    // Zero-crossing center line
    g.setColour(juce::Colour(0x33ffffff));
    g.drawHorizontalLine(static_cast<int>(centerY), bounds.getX(), bounds.getRight());

    // Selection Overlay
    float selStartX = normalizedToX(selectionStartNormalized);
    float selEndX   = normalizedToX(selectionEndNormalized);
    float selWidth  = std::max(1.0f, selEndX - selStartX);

    // Shaded selection box
    g.setColour(juce::Colour(0x2800e5ff));
    g.fillRect(selStartX, bounds.getY(), selWidth, bounds.getHeight());

    // Selection bounding lines & handles
    g.setColour(juce::Colour(0xff00e5ff));
    g.drawVerticalLine(static_cast<int>(selStartX), bounds.getY(), bounds.getBottom());
    g.drawVerticalLine(static_cast<int>(selEndX), bounds.getY(), bounds.getBottom());

    // Handles at top & bottom corners
    g.fillRect(selStartX - 3.0f, bounds.getY(), 6.0f, 12.0f);
    g.fillRect(selStartX - 3.0f, bounds.getBottom() - 12.0f, 6.0f, 12.0f);
    g.fillRect(selEndX - 3.0f, bounds.getY(), 6.0f, 12.0f);
    g.fillRect(selEndX - 3.0f, bounds.getBottom() - 12.0f, 6.0f, 12.0f);

    // Audition Playhead
    if (buffer.isAuditioning())
    {
        double progress = buffer.getAuditionProgress();
        float playheadX = selStartX + static_cast<float>(progress * selWidth);

        g.setColour(juce::Colour(0xffff9100)); // Vivid Amber Playhead
        g.drawVerticalLine(static_cast<int>(playheadX), bounds.getY(), bounds.getBottom());
        g.fillEllipse(playheadX - 4.0f, bounds.getY() + 2.0f, 8.0f, 8.0f);
    }

    // "NOW" Head indicator (Right edge)
    g.setColour(buffer.isRecording() ? juce::Colour(0xffff1744) : juce::Colour(0xff757575));
    g.drawVerticalLine(static_cast<int>(bounds.getRight() - 1.0f), bounds.getY(), bounds.getBottom());

    // Outer border
    g.setColour(juce::Colour(0xff2a2d3d));
    g.drawRect(bounds, 1.0f);

    // --- Timeline Ruler Rendering ---
    g.fillAll(juce::Colour(0xff0e0f14)); // ruler background
    g.setFont(juce::FontOptions(11.0f));
    g.setColour(juce::Colour(0x88ffffff));

    for (double sec = 0.0; sec <= visSec; sec += gridIntervalSec)
    {
        float norm = static_cast<float>(1.0 - (sec / visSec));
        float gx = normalizedToX(norm);

        juce::String timeLabel;
        if (sec == 0.0)
            timeLabel = "NOW";
        else if (sec >= 60.0)
            timeLabel = "-" + juce::String(static_cast<int>(sec / 60.0)) + "m";
        else
            timeLabel = "-" + juce::String(static_cast<int>(sec)) + "s";

        g.drawText(timeLabel, static_cast<int>(gx) - 24, static_cast<int>(timelineBounds.getY()), 48, static_cast<int>(timelineBounds.getHeight()), juce::Justification::centred);
    }
}

} // namespace RecRoll
