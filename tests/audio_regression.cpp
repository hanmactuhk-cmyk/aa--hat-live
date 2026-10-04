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
  MicAnalysis quiet,loud,silence;quiet.reset(rate);loud.reset(rate);silence.reset(rate);for(int i=0;i<int(rate);++i){float x=std::sin(float(2*3.141592653589793*220*i/rate));quiet.feed(.04f*x);loud.feed(.4f*x);silence.feed(0);}auto q=quiet.result(),l=loud.result();require(q.valid&&l.valid&&!silence.result().valid,"Auto Vocal did not reject silence or accept voice");require(q.micVolume>l.micVolume,"Auto Vocal gain did not react to level");require(q.compressorDb<l.compressorDb,"Auto Vocal compressor did not react to level");
 }
 auto mic=BusMixer::sum(.2f,.3f,{.4f,.5f},1,1,1,1),music=BusMixer::sum(.2f,.3f,{.4f,.5f},1,1,1,2);require(mic[0]==.2f&&music[0]==.4f,"Source listening isolation failed");
 std::cout<<"Audio regression passed: music bypasses vocal DSP/VST, MIC/MUSIC solo, 450Hz corrected to A440, Auto Vocal follows microphone level at 44.1/48/96kHz\n";
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
