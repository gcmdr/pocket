# Pocket Plugin

This is a JUCE audio plugin designed to provide visual feedback on musical timing accuracy. It analyzes incoming MIDI notes against the host's transport information to display whether the timing is early or late.

## Features

*   Displays the timing difference in milliseconds (ms) for early and late notes.
*   Displays the current playback position in PPQ (Pulses Per Quarter Note).

## Building

This project uses the JUCE framework. To build it:
1.  Open the `.jucer` file in the Projucer application.
2.  Select your target IDE (e.g., Visual Studio, Xcode).
3.  Save the project and open it in your chosen IDE.
4.  Build the plugin target (VST3, AU, Standalone, etc.). 