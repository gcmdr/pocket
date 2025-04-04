/*
  ==============================================================================

   This file is part of the JUCE framework.
   Copyright (c) Raw Material Software Limited

   JUCE is an open source framework subject to commercial or open source
   licensing.

   By downloading, installing, or using the JUCE framework, or combining the
   JUCE framework with any other source code, object code, content or any other
   copyrightable work, you agree to the terms of the JUCE End User Licence
   Agreement, and all incorporated terms including the JUCE Privacy Policy and
   the JUCE Website Terms of Service, as applicable, which will bind you. If you
   do not agree to the terms of these agreements, we will not license the JUCE
   framework to you, and you must discontinue the installation or download
   process and cease use of the JUCE framework.

   JUCE End User Licence Agreement: https://juce.com/legal/juce-8-licence/
   JUCE Privacy Policy: https://juce.com/juce-privacy-policy
   JUCE Website Terms of Service: https://juce.com/juce-website-terms-of-service/

   Or:

   You may also use this code under the terms of the AGPLv3:
   https://www.gnu.org/licenses/agpl-3.0.en.html

   THE JUCE FRAMEWORK IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL
   WARRANTIES, WHETHER EXPRESSED OR IMPLIED, INCLUDING WARRANTY OF
   MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

namespace juce::dsp
{

//==============================================================================
template <typename SampleType>
Panner<SampleType>::Panner()
{
    update();
    reset();
}

//==============================================================================
template <typename SampleType>
void Panner<SampleType>::setRule (Rule newRule)
{
    currentRule = newRule;
    update();
}

template <typename SampleType>
void Panner<SampleType>::setPan (SampleType newPan)
{
    jassert (newPan >= -1.0 && newPan <= 1.0);

    pan = jlimit (static_cast<SampleType> (-1.0), static_cast<SampleType> (1.0), newPan);
    update();
}

//==============================================================================
template <typename SampleType>
void Panner<SampleType>::prepare (const ProcessSpec& spec)
{
    jassert (spec.sampleRate > 0);
    jassert (spec.numChannels > 0);

    sampleRate = spec.sampleRate;

    reset();
}

template <typename SampleType>
void Panner<SampleType>::reset()
{
    leftVolume .reset (sampleRate, 0.05);
    rightVolume.reset (sampleRate, 0.05);
}

//==============================================================================
template <typename SampleType>
void Panner<SampleType>::update()
{
    SampleType leftValue, rightValue, boostValue;

    auto normalisedPan = static_cast<SampleType> (0.5) * (pan + static_cast<SampleType> (1.0));

    switch (currentRule)
    {
        case Rule::balanced:
            leftValue  = jmin (static_cast<SampleType> (0.5), static_cast<SampleType> (1.0) - normalisedPan);
            rightValue = jmin (static_cast<SampleType> (0.5), normalisedPan);
            boostValue = static_cast<SampleType> (2.0);
            break;

        case Rule::linear:
            leftValue  = static_cast<SampleType> (1.0) - normalisedPan;
            rightValue = normalisedPan;
            boostValue = static_cast<SampleType> (2.0);
            break;

        case Rule::sin3dB:
            leftValue  = static_cast<SampleType> (std::sin (0.5 * MathConstants<double>::pi * (1.0 - normalisedPan)));
            rightValue = static_cast<SampleType> (std::sin (0.5 * MathConstants<double>::pi * normalisedPan));
            boostValue = std::sqrt (static_cast<SampleType> (2.0));
            break;

        case Rule::sin4p5dB:
            leftValue  = static_cast<SampleType> (std::pow (std::sin (0.5 * MathConstants<double>::pi * (1.0 - normalisedPan)), 1.5));
            rightValue = static_cast<SampleType> (std::pow (std::sin (0.5 * MathConstants<double>::pi * normalisedPan), 1.5));
            boostValue = static_cast<SampleType> (std::pow (2.0, 3.0 / 4.0));
            break;

        case Rule::sin6dB:
            leftValue  = static_cast<SampleType> (std::pow (std::sin (0.5 * MathConstants<double>::pi * (1.0 - normalisedPan)), 2.0));
            rightValue = static_cast<SampleType> (std::pow (std::sin (0.5 * MathConstants<double>::pi * normalisedPan), 2.0));
            boostValue = static_cast<SampleType> (2.0);
            break;

        case Rule::squareRoot3dB:
            leftValue  = std::sqrt (static_cast<SampleType> (1.0) - normalisedPan);
            rightValue = std::sqrt (normalisedPan);
            boostValue = std::sqrt (static_cast<SampleType> (2.0));
            break;

        case Rule::squareRoot4p5dB:
            leftValue  = static_cast<SampleType> (std::pow (std::sqrt (1.0 - normalisedPan), 1.5));
            rightValue = static_cast<SampleType> (std::pow (std::sqrt (normalisedPan), 1.5));
            boostValue = static_cast<SampleType> (std::pow (2.0, 3.0 / 4.0));
            break;

        default:
            leftValue  = jmin (static_cast<SampleType> (0.5), static_cast<SampleType> (1.0) - normalisedPan);
            rightValue = jmin (static_cast<SampleType> (0.5), normalisedPan);
            boostValue = static_cast<SampleType> (2.0);
            break;
    }

    leftVolume .setTargetValue (leftValue  * boostValue);
    rightVolume.setTargetValue (rightValue * boostValue);
}

//==============================================================================
template class Panner<float>;
template class Panner<double>;

} // namespace juce::dsp
