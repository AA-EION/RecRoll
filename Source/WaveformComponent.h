#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "RollingBuffer.h"

namespace RecRoll
{

class WaveformComponent : public juce::Component,
                          public juce::Timer,
                          public juce::DragAndDropContainer
{
public:
    WaveformComponent(RollingBuffer& buffer);
    ~WaveformComponent() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    void timerCallback() override;

    void mouseDown(const juce::MouseEvent& e) override;
    void mouseDrag(const juce::MouseEvent& e) override;
    void mouseUp(const juce::MouseEvent& e) override;
    void mouseDoubleClick(const juce::MouseEvent& e) override;
    void mouseEnter(const juce::MouseEvent& e) override;
    void mouseExit(const juce::MouseEvent& e) override;
    void mouseMove(const juce::MouseEvent& e) override;

    /** Freezes visual waveform scrolling at the current moment without halting recording. */
    void freezeView();

    /** Resumes live visual scrolling. */
    void unfreezeView();

    /** Called when an external drag-and-drop operation completes. */
    void onDragOperationEnded();

    bool isViewCurrentlyFrozen() const noexcept { return isViewFrozen; }

    /** Sets whether normalization is applied when dragging to DAW. */
    void setNormalizeExport(bool shouldNormalize) { normalizeExport = shouldNormalize; }
    bool getNormalizeExport() const noexcept { return normalizeExport; }

    /** Selects the entire currently visible buffer. */
    void selectAll();

    /** Clears current selection (defaults to entire buffer). */
    void clearSelection();

    /** Toggles auditioning of the current selection. */
    void toggleAudition();

    /** Call this to initiate an external drag-and-drop to DAW or OS file explorer. */
    void startDawDragOperation();

    /** Callback to inform parent of selection time changes for UI readouts. */
    std::function<void(double startSec, double endSec, double durationSec)> onSelectionChanged;

private:
    RollingBuffer& buffer;
    std::vector<RollingBuffer::PeakData> peakCache;

    bool normalizeExport { false };

    // Visual view freeze state (scrolling pauses, recording continues)
    bool isViewFrozen { false };
    int64_t frozenEndSample { -1 };

    // Selection range normalized to [0.0, 1.0] where 0.0 is oldest (left) and 1.0 is newest (right / Now)
    double selectionStartNormalized { 0.0 };
    double selectionEndNormalized { 1.0 };
    bool hasCustomSelection { false };

    // Mouse drag state
    enum class DragMode
    {
        None,
        CreatingSelection,
        MovingStartHandle,
        MovingEndHandle,
        ExportingToDaw
    };

    DragMode currentDragMode { DragMode::None };
    juce::Point<int> dragStartPos;
    bool hasInitiatedExternalDrag { false };

    // UI geometry & caching
    juce::Rectangle<float> waveformBounds;
    juce::Rectangle<float> timelineBounds;

    double xToNormalized(float x) const;
    float normalizedToX(double norm) const;

    void updateSelectionReadout();

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(WaveformComponent)
};

} // namespace RecRoll
