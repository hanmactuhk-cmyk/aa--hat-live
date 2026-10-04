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
    // Reverb must generate a decaying tail, preserve the dry onset and remain stable.
    VocalDSP room;room.prepare(48000);room.gate=room.compressor=room.equalizer=room.deesser=room.autoKey=false;room.wet=.4f;room.reverbDecay=1.6f;
    double early=0,late=0;float onset=room.process(.5f);
    if(std::abs(onset-.5f)>1e-6)return 4;
    for(int i=1;i<48000*6;++i){float v=room.process(0);if(!std::isfinite(v)||std::abs(v)>1)return 5;if(i<48000)early+=v*v;if(i>48000*5)late+=v*v;}
    if(early<1e-6||late>early*.01)return 6;
    // Low-cut must attenuate rumble while warmth/air remain audible and bounded.
    VocalDSP clean;clean.prepare(48000);clean.gate=clean.compressor=clean.equalizer=clean.deesser=clean.reverb=clean.autoKey=false;clean.highpass=true;clean.highpassHz=180;
    double power=0;for(int i=0;i<48000;++i){float v=clean.process(.2f*std::sin(float(6.28318530718*40*i/48000)));if(i>24000)power+=v*v;}if(power>25)return 7;
    clean.warmth=clean.air=true;for(int i=0;i<48000;++i){float v=clean.process(.2f*std::sin(float(6.28318530718*440*i/48000)));if(!std::isfinite(v)||std::abs(v)>1)return 8;}
    if(neuralPitchConfidence(.99f,.1f,.073f,440)<.55f)return 9;
    std::cout << "DSP self-test passed: EQ, pitch/Auto Key, reverb, silence gate at 44.1/48/96 kHz\n";
    return 0;
}
