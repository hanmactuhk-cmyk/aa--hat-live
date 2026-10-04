#pragma once
#include <array>
// Vocal processing owns only the mic bus. Music never enters the DSP/VST buffer.
struct BusMixer {
 static std::array<float,2> sum(float vocalL,float vocalR,const std::array<float,2>& music,float micGain,float musicGain,float masterGain,int listenMode=0){
  if(listenMode==2)micGain=0; // music only
  if(listenMode==1)musicGain=0; // mic only
  return {(vocalL*micGain+music[0]*musicGain)*masterGain,(vocalR*micGain+music[1]*musicGain)*masterGain};
 }
 template<class MicReader,class VocalWriter,class Processor>
 static void vocal(int samples,MicReader read,VocalWriter write,Processor& dsp){for(int i=0;i<samples;++i)write(i,dsp.process(read(i)));}
};
