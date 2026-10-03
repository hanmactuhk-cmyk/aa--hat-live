#include "../native/dsp.h"
#include <iostream>

int main() {
    for (double rate : {44100.0, 48000.0, 96000.0}) {
        VocalDSP dsp;
        dsp.prepare(rate);
        for (size_t i = 0; i < 13; ++i)
            dsp.eq[i].peak(rate, VocalDSP::hz[i], (i % 2) ? 6.0 : -6.0);
        dsp.tune = true;
        dsp.autoKey = true;
        double energy = 0;
        for (int i = 0; i < static_cast<int>(rate * 2); ++i) {
            const float x = .2f * std::sin(static_cast<float>(2 * 3.141592653589793 * 440 * i / rate));
            const float y = dsp.process(x);
            if (!std::isfinite(y) || std::abs(y) > 5) {
                std::cerr << "FAIL: invalid DSP output at " << rate << " Hz, sample " << i << " (" << y << ")\n";
                return 1;
            }
            energy += static_cast<double>(y) * y;
        }
        if (energy < .01) {
            std::cerr << "FAIL: DSP output is silent at " << rate << " Hz\n";
            return 2;
        }
    }
    VocalDSP silent;
    silent.prepare(48000);
    for (int i = 0; i < 48000; ++i) {
        if (silent.process(0) != 0) {
            std::cerr << "FAIL: gate produced output from silence at sample " << i << "\n";
            return 3;
        }
    }
    std::cout << "DSP self-test passed: EQ, pitch/Auto Key, reverb, silence gate at 44.1/48/96 kHz\n";
    return 0;
}
