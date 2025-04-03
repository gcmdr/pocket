/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PocketAudioProcessorEditor::PocketAudioProcessorEditor (PocketAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    // Setup the timing labels and divider
    // timingLabel.setText ("-- ms", juce::dontSendNotification); // <-- REMOVED
    // timingLabel.setFont (juce::Font (18.0f)); // <-- REMOVED
    // timingLabel.setJustificationType (juce::Justification::centred); // <-- REMOVED
    // addAndMakeVisible (timingLabel); // <-- REMOVED

    earlyMsLabel.setText ("", juce::dontSendNotification);
    earlyMsLabel.setFont (juce::Font (18.0f));
    earlyMsLabel.setJustificationType (juce::Justification::centredRight); // Align to right
    addAndMakeVisible (earlyMsLabel);

    dividerLabel.setText ("|", juce::dontSendNotification);
    dividerLabel.setFont (juce::Font (18.0f));
    dividerLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (dividerLabel);

    lateMsLabel.setText ("", juce::dontSendNotification);
    lateMsLabel.setFont (juce::Font (18.0f));
    lateMsLabel.setJustificationType (juce::Justification::centredLeft); // Align to left
    addAndMakeVisible (lateMsLabel);

    // Setup the playhead label
    playheadLabel.setText ("Stopped", juce::dontSendNotification);
    playheadLabel.setFont (juce::Font (14.0f));
    playheadLabel.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (playheadLabel);

    // Set editor size
    setSize (400, 120);

    startTimerHz(30);
}

PocketAudioProcessorEditor::~PocketAudioProcessorEditor()
{
    stopTimer();
}

//==============================================================================
void PocketAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PocketAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto timingArea = bounds.removeFromTop(bounds.getHeight() / 2);
    playheadLabel.setBounds (bounds); // Playhead takes bottom half

    // Divide timing area horizontally
    auto dividerWidth = 20;
    auto sideWidth = (timingArea.getWidth() - dividerWidth) / 2;

    earlyMsLabel.setBounds(timingArea.removeFromLeft(sideWidth));
    dividerLabel.setBounds(timingArea.removeFromLeft(dividerWidth));
    lateMsLabel.setBounds(timingArea); // Takes the rest
}

void PocketAudioProcessorEditor::timerCallback()
{
    // --- Update Timing Labels ---
    double differenceMs = audioProcessor.lastTimingDifferenceMs.load();
    juce::String earlyString = "";
    juce::String lateString = "";
    const double threshold = 0.001; // To avoid showing tiny values

    if (differenceMs < -threshold) // Rushing
    {
        // Display absolute value, aligned right
        earlyString = juce::String(std::abs(differenceMs), 1) + " ms";
    }
    else if (differenceMs > threshold) // Dragging
    {
        // Display value, aligned left
        lateString = "+ " + juce::String(differenceMs, 1) + " ms";
    }
    // If within threshold, both strings remain empty ("")

    // Use MessageManager to safely update UI from timer thread
    juce::MessageManager::callAsync([this, earlyString, lateString]() {
        // timingLabel.setText(differenceString, juce::dontSendNotification); // <-- REMOVED
        earlyMsLabel.setText(earlyString, juce::dontSendNotification);
        lateMsLabel.setText(lateString, juce::dontSendNotification);
    });

    // --- Update Playhead Label ---
    double ppq = audioProcessor.currentPpqPosition.load();
    juce::String playheadString;
    if (ppq >= 0.0)
    {
        playheadString = "PPQ: " + juce::String(ppq, 3);
    }
    else
    {
        playheadString = "Stopped";
    }
    juce::MessageManager::callAsync([this, playheadString]() {
         playheadLabel.setText(playheadString, juce::dontSendNotification);
    });
}
