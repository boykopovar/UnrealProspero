#include "radaudio_decoder.h"
#include <algorithm>
#include <cmath>

void RadAudioInterleave(S16* output, const float** inputs, U32 inputCount, U32 samples)
{
    for (U32 sample = 0; sample < samples; ++sample)
    {
        for (U32 channel = 0; channel < inputCount; ++channel)
        {
            const float value = inputs[channel][sample];
            const float scaled = std::clamp(value * 32768.0f, -32768.0f, 32767.0f);
            *output++ = std::isnan(scaled) ? 0 : static_cast<S16>(std::round(scaled));
        }
    }
}
