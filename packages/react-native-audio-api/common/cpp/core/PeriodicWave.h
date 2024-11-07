#pragma once

namespace audioapi {
    class PeriodicWave {

    private:
        explicit PeriodicWave(unsigned sampleRate);

        // determines the time resolution of the waveform.
        unsigned sampleRate_;
        // determines number of frequency segments (or bands) the signal is divided.
        unsigned numberOfRanges_;
        // the lowest frequency (in hertz) where playback will include all of the partials.
        float lowestFundamentalFrequency;
        // scaling factor used to adjust size of period of waveform to the sample rate.
        float rateScale_;
    };
} // namespace audioapi
