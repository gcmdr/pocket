/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <JuceHeader.h>
#if JUCE_PROJUCER_VERSION < 0x070000
 #include "JucePluginDefines.h"
#endif

//==============================================================================
PocketAudioProcessor::PocketAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

PocketAudioProcessor::~PocketAudioProcessor()
{
}

//==============================================================================
const juce::String PocketAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PocketAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PocketAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PocketAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PocketAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PocketAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int PocketAudioProcessor::getCurrentProgram()
{
    return 0;
}

void PocketAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String PocketAudioProcessor::getProgramName (int index)
{
    return {};
}

void PocketAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void PocketAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
}

void PocketAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool PocketAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void PocketAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // --- Start of Timing Logic ---

    const double sampleRate = getSampleRate();
    juce::AudioPlayHead* playHead = getPlayHead();
    juce::AudioPlayHead::CurrentPositionInfo positionInfo;

    // Attempt to get position info. Proceed only if successful and playing.
    if (playHead != nullptr && playHead->getCurrentPosition(positionInfo) && positionInfo.isPlaying)
    {
        currentPpqPosition.store(positionInfo.ppqPosition);

        // Check if tempo is valid (greater than zero)
        if (positionInfo.bpm > 0)
        {
            const double ppqPerMinute = positionInfo.bpm;
            const double startPpq = positionInfo.ppqPosition;

            for (const auto metadata : midiMessages)
            {
                const juce::MidiMessage message = metadata.getMessage();

                if (message.isNoteOn())
                {
                    const double secondsIntoBuffer = metadata.samplePosition / sampleRate;
                    const double ppqOffset = secondsIntoBuffer * (ppqPerMinute / 60.0);
                    const double noteAbsolutePpq = startPpq + ppqOffset;
                    const double nearestQuarterNotePpq = std::round(noteAbsolutePpq);
                    const double ppqDifference = noteAbsolutePpq - nearestQuarterNotePpq;
                    const double msDifference = ppqDifference * (60000.0 / ppqPerMinute);
                    lastTimingDifferenceMs.store(msDifference);
                }
            }
        }
        else // BPM is not positive
        {
             lastTimingDifferenceMs.store(0.0);
        }
    }
    else // If not playing or playhead unavailable
    {
        currentPpqPosition.store(-1.0);
        lastTimingDifferenceMs.store(0.0);
    }
    // --- End of Timing Logic ---


    // Standard audio processing loop (can be left empty for MIDI effect)
    // If this is intended as a MIDI effect, you might not need this audio loop.
    // If it processes audio, keep it.
    /*
    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        // ..do something to the data...
        juce::ignoreUnused(channelData); // If you're not using channelData
    }
    */

    // If this is a MIDI effect, you might want to clear the MIDI buffer if you've processed
    // everything, or pass messages through, or generate new ones.
    // For now, we'll leave midiMessages unmodified to pass MIDI through.
}

//==============================================================================
bool PocketAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* PocketAudioProcessor::createEditor()
{
    return new PocketAudioProcessorEditor (*this);
}

//==============================================================================
void PocketAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void PocketAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PocketAudioProcessor();
}
