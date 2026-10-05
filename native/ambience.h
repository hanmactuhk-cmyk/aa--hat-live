#pragma once
#include <array>
#include <vector>
#include <cmath>
#include <algorithm>
// Stereo post-insert ambience. Prepared off the audio thread; process allocates nothing.
struct StereoAmbience {
 double sr=48000;bool reverb=true,shortEcho=false,longEcho=false,delay=false,sync=false,pingPong=true;
 float wet=.18f,decay=1.8f,damping=.55f,preMs=25,width=1,shortWet=.15f,longWet=.18f,shortMs=120,longMs=380,shortFeedback=.2f,longFeedback=.4f;
 float delayWet=.2f,delayMs=380,feedback=.32f,bpm=120,division=1,duck=.45f,tone=.55f;
 std::array<std::vector<float>,8> room;std::array<unsigned,8> pos{};std::array<float,8> filtered{},gain{};
 std::array<std::vector<float>,2> pre;unsigned prePos=0;
 std::array<std::array<std::vector<float>,2>,3> echo;unsigned echoPos=0;std::array<std::array<float,2>,3> echoLow{},echoHigh{};
 std::array<float,2> roomLow{};float envelope=0,release=0,echoLP=0,wetHP=0;unsigned delaySamples=1,preSamples=0;
 void configure(){release=float(1-std::exp(-1/(sr*.12)));echoLP=float(1-std::exp(-6.28318530718*(1800+(1-tone)*8500)/sr));wetHP=float(1-std::exp(-6.28318530718*160/sr));delaySamples=unsigned(std::clamp(sr*(sync?60000/std::max(40.f,bpm)*division:delayMs)/1000,1.,sr*3.0));preSamples=unsigned(std::clamp(sr*preMs/1000,0.,sr*.12));for(unsigned i=0;i<8;++i)gain[i]=float(std::pow(.001,room[i].size()/sr/std::max(.25f,decay)));}
 void prepare(double rate){sr=rate;static constexpr double lengths[]{.0297,.0371,.0419,.0437,.0533,.0593,.0677,.0739};for(unsigned i=0;i<8;++i){room[i].assign(size_t(sr*lengths[i])+1,0);pos[i]=0;}for(auto& p:pre)p.assign(size_t(sr*.121)+2,0);for(auto& bank:echo)for(auto& p:bank)p.assign(size_t(sr*3.1)+2,0);filtered.fill(0);roomLow.fill(0);for(auto& p:echoLow)p.fill(0);for(auto& p:echoHigh)p.fill(0);prePos=echoPos=0;envelope=0;configure();}
 std::array<float,2> process(float l,float r){std::array<float,2> dry{std::isfinite(l)?l:0,std::isfinite(r)?r:0},out=dry;if(pre[0].empty())return out;float peak=std::max(std::abs(dry[0]),std::abs(dry[1]));envelope+=(peak>envelope?.01f:release)*(peak-envelope);float duckGain=1/(1+duck*envelope*12);
  std::array<float,2> roomWet{};unsigned pn=unsigned(pre[0].size());for(int ch=0;ch<2;++ch)pre[ch][prePos]=reverb?dry[ch]:0;
  std::array<float,8> z{};for(unsigned i=0;i<8;++i){float old=room[i][pos[i]];filtered[i]+=(1-damping*.94f)*(old-filtered[i]);z[i]=filtered[i];roomWet[i&1]+=z[i]*.25f;}
  // Orthogonal Hadamard feedback: dense diffusion without runaway gain.
  for(unsigned span=1;span<8;span*=2)for(unsigned base=0;base<8;base+=span*2)for(unsigned j=0;j<span;++j){float a=z[base+j],b=z[base+j+span];z[base+j]=a+b;z[base+j+span]=a-b;}
  for(unsigned i=0;i<8;++i){float input=pre[i&1][(prePos+pn-preSamples)%pn];room[i][pos[i]]=reverb?input*.25f+z[i]*.35355339f*gain[i]:0;pos[i]=(pos[i]+1)%unsigned(room[i].size());}prePos=(prePos+1)%pn;
  for(int ch=0;ch<2;++ch){roomLow[ch]+=wetHP*(roomWet[ch]-roomLow[ch]);roomWet[ch]-=roomLow[ch];}float mid=(roomWet[0]+roomWet[1])*.5f,side=(roomWet[0]-roomWet[1])*.5f*width;if(reverb){out[0]+=(mid+side)*wet;out[1]+=(mid-side)*wet;}
  unsigned en=unsigned(echo[0][0].size()),ep=echoPos%en;for(unsigned bank=0;bank<3;++bank){bool on=bank==0?shortEcho:bank==1?longEcho:delay;unsigned offset=bank==2?delaySamples:unsigned(sr*(bank==0?shortMs:longMs)/1000);offset=std::clamp(offset,1u,en-1);float f=bank==0?shortFeedback:bank==1?longFeedback:feedback;float amount=bank==0?shortWet:bank==1?longWet:delayWet;std::array<float,2> delayed{};for(int ch=0;ch<2;++ch){float v=echo[bank][ch][(ep+en-offset)%en];echoLow[bank][ch]+=echoLP*(v-echoLow[bank][ch]);echoHigh[bank][ch]+=wetHP*(echoLow[bank][ch]-echoHigh[bank][ch]);delayed[ch]=echoLow[bank][ch]-echoHigh[bank][ch];}
   for(int ch=0;ch<2;++ch){float input=dry[ch];if(bank==2&&pingPong)input=ch==0?(dry[0]+dry[1])*.5f:0;float back=delayed[bank==2&&pingPong?1-ch:ch];echo[bank][ch][ep]=on?input+std::tanh(back)*f:0;if(on)out[ch]+=delayed[ch]*amount*(bank==2?duckGain:1);}}
  echoPos=(ep+1)%en;for(auto& v:out)if(!std::isfinite(v))v=0;return out;
 }
};
