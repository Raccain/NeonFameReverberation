#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ParameterIDs
{
#define PARAMETER_ID(str) static const juce::ParameterID str { #str, 1 };

    PARAMETER_ID (mix)
    PARAMETER_ID (decay)
    PARAMETER_ID (tension)
    PARAMETER_ID (pre_delay)
    PARAMETER_ID (damping)
    PARAMETER_ID (wobble)
    PARAMETER_ID (drive)

#undef PARAMETER_ID
}
