#pragma once
#include <algorithm>
#include <array>
#include <stdexcept>

// Channel masks enable every channel through the highest selected index.
// This deliberately keeps JUCE's compact callback arrays aligned to port indices.
struct Routing {
 bool asio=false,sendEnabled=false;
 int mic=0,musicLeft=2,musicRight=3,monitorLeft=0,monitorRight=1,sendLeft=2,sendRight=3;
 int requiredInputs()const{return asio?std::max({mic,musicLeft,musicRight})+1:1;}
 int requiredOutputs()const{return std::max({monitorLeft,monitorRight,sendEnabled?sendLeft:0,sendEnabled?sendRight:0})+1;}
 void validate(int inputs,int outputs)const{
  if(monitorLeft<0||monitorRight<0||monitorLeft==monitorRight||requiredOutputs()>outputs)throw std::runtime_error("Invalid Master output ports");
  if(asio&&(mic<0||musicLeft<0||musicRight<0||musicLeft==musicRight||mic==musicLeft||mic==musicRight||requiredInputs()>inputs))throw std::runtime_error("MIC and MUSIC need separate available ASIO input ports");
  if(sendEnabled&&(sendLeft<0||sendRight<0||sendLeft==sendRight||sendLeft==monitorLeft||sendLeft==monitorRight||sendRight==monitorLeft||sendRight==monitorRight))throw std::runtime_error("Second Master output must use a separate stereo pair");
 }
 static float read(const float* const* channels,int count,int channel,int sample){return channel>=0&&channel<count&&channels[channel]?channels[channel][sample]:0.f;}
 std::array<float,2> music(const float* const* channels,int count,int sample)const{return {read(channels,count,musicLeft,sample),read(channels,count,musicRight,sample)};}
 void write(float* const* channels,int count,int sample,float left,float right)const{
  auto put=[&](int index,float value){if(index>=0&&index<count&&channels[index])channels[index][sample]=value;};
  put(monitorLeft,left);put(monitorRight,right);
  if(sendEnabled){put(sendLeft,left);put(sendRight,right);}
 }
};
