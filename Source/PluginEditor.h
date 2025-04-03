/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

//==============================================================================
/**
*/
class PocketAudioProcessorEditor  : public juce::AudioProcessorEditor,
                                  private juce::Timer
{
public:
    PocketAudioProcessorEditor (PocketAudioProcessor&);
    ~PocketAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

    //==============================================================================
    // Timer callback
    void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    PocketAudioProcessor& audioProcessor;

    // UI Components for Positional Timing Feedback
    // juce::Label timingLabel; // <-- REMOVED
    juce::Label earlyMsLabel;   // Displays timing when rushing (negative ms)
    juce::Label dividerLabel;   // Fixed divider "|"
    juce::Label lateMsLabel;    // Displays timing when dragging (positive ms)

    juce::Label playheadLabel;  // Existing label for playhead info

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PocketAudioProcessorEditor)
};
