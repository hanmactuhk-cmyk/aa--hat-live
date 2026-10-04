#include "../native/buses.h"
#include "../native/dsp.h"
#include "../native/mic_analysis.h"
#include <iostream>
#include <stdexcept>
void require(bool ok,const char* message){if(!ok)throw std::runtime_error(message);}
int main(){try{
 for(double rate:{44100.,48000.,96000.}){
  // Loud vocal EQ/reverb/pitch + a deliberately destructive fake VST insert.
  // Music must remain sample-identical when the mic source is silent.
  VocalDSP d;d.prepare(rate);d.tune=d.autoKey=true;d.wet=.9f;for(auto& band:d.eq)band.peak(rate,1000,18);
  std::array<float,256> vocal{},musicL{},musicR{};
  for(int block=0;block<25;++block){BusMixer::vocal(256,[](int){return 0.f;},[&](int i,float value){vocal[i]=value;},d);for(int i=0;i<256;++i){vocal[i]=std::tanh(vocal[i]*20);musicL[i]=.2f*std::sin(float(i+block*256)*.17f);musicR[i]=-.13f*std::cos(float(i+block*256)*.23f);auto out=BusMixer::sum(vocal[i],vocal[i],{musicL[i],musicR[i]},1,1,1);require(out[0]==musicL[i]&&out[1]==musicR[i],"Music entered the vocal FX chain");}}
  // Pitch correction is checked at the actual output, not only the detected note.
  VocalDSP pitch;pitch.prepare(rate);pitch.gate=pitch.compressor=pitch.equalizer=pitch.deesser=pitch.reverb=false;pitch.tune=true;pitch.chromatic=true;pitch.tuneStrength=1;int crossings=0;float last=0;
  for(int i=0;i<int(rate*3);++i){float y=pitch.process(.2f*std::sin(float(2*3.141592653589793*450*i/rate)));if(i>rate&&y>=0&&last<0)++crossings;last=y;}
  require(std::abs(pitch.detectedHz.load()-450)<2,"Pitch detector missed 450Hz");require(std::abs(pitch.targetHz.load()-440)<.1,"Pitch target was not A440");require(std::abs(crossings/2.-440)<3,"Pitch correction did not change actual output to A440");
  // The detector must reject a single-note key guess, then identify a multi-note C major melody.
  VocalDSP key;key.prepare(rate);key.gate=key.compressor=key.equalizer=key.deesser=key.reverb=false;key.autoKey=true;
  for(int i=0;i<int(rate*2);++i)key.process(.2f*std::sin(float(6.28318530718*440*i/rate)));require(!key.keyReady.load(),"Auto Key locked a key from only one note");key.resetKey();
  int melody[]{60,64,67,72,71,69,67,65,64,62,60,60,64,67,60,72};for(int n:melody){double f=440*std::pow(2.,(n-69)/12.);for(int i=0;i<int(rate*.4);++i)key.process(.2f*std::sin(float(6.28318530718*f*i/rate)));}require(key.keyReady.load()&&key.detectedKey.load()==0,"Auto Key did not detect C major melody");
  for(bool shortMode:{true,false}){VocalDSP echo;echo.prepare(rate);echo.gate=echo.compressor=echo.equalizer=echo.deesser=echo.reverb=echo.autoKey=false;echo.shortEcho=shortMode;echo.longEcho=!shortMode;echo.shortWet=echo.longWet=.5;echo.shortMs=120;echo.longMs=380;unsigned delay=unsigned(rate*(shortMode?120:380)/1000);for(unsigned i=0;i<=delay;++i){float y=echo.process(i==0?.2f:0.f);if(i==delay)require(std::abs(y-.1f)<.0001,"Short/long echo did not produce the delayed mic impulse");else if(i>0)require(std::abs(y)<.0001,"Echo produced a premature repeat");}}
  VocalDSP meter;meter.prepare(rate);meter.gate=meter.compressor=meter.equalizer=meter.deesser=meter.reverb=meter.autoKey=false;for(int i=0;i<int(rate);++i)meter.process(.2f*std::sin(float(6.28318530718*1000*i/rate)));meter.publishSpectrum();require(meter.eqLevels[7].load()>meter.eqLevels[0].load()*5,"EQ spectrum did not react to a real 1kHz microphone tone");
  MicAnalysis quiet,loud,silence;quiet.reset(rate);loud.reset(rate);silence.reset(rate);for(int i=0;i<int(rate);++i){float x=std::sin(float(2*3.141592653589793*220*i/rate));quiet.feed(.04f*x);loud.feed(.4f*x);silence.feed(0);}auto q=quiet.result(),l=loud.result();require(q.valid&&l.valid&&!silence.result().valid,"Auto Vocal did not reject silence or accept voice");require(q.micVolume>l.micVolume,"Auto Vocal gain did not react to level");require(q.compressorDb<l.compressorDb,"Auto Vocal compressor did not react to level");
 }
 auto mic=BusMixer::sum(.2f,.3f,{.4f,.5f},1,1,1,1),music=BusMixer::sum(.2f,.3f,{.4f,.5f},1,1,1,2);require(mic[0]==.2f&&music[0]==.4f,"Source listening isolation failed");
 std::cout<<"Audio regression passed: music bypasses vocal DSP/VST, MIC/MUSIC solo, 450Hz corrected to A440, Auto Vocal follows microphone level, automatic C-major detection, short/long echo and actual EQ levels at 44.1/48/96kHz\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
