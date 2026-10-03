#pragma once
#include <array>
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>
// Bounded-memory vocal DSP. No allocations or IPC in process().
struct Biquad {
 float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void peak(double sr,double hz,double db){double A=std::pow(10.,db/40.),w=2*3.141592653589793*std::min(hz,sr*.45)/sr,alpha=std::sin(w)/2;double a0=1+alpha/A;b0=float((1+alpha*A)/a0);b1=float(-2*std::cos(w)/a0);b2=float((1-alpha*A)/a0);a1=b1;a2=float((1-alpha/A)/a0);}
 float process(float x){float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
};
struct VocalDSP {
 double sr=48000;std::array<Biquad,13> eq{};
 static constexpr std::array<double,13> hz{40,63,100,160,250,400,630,1000,1600,2500,4000,6300,10000};
 bool gate=true,compressor=true,equalizer=true,deesser=true,tune=false,reverb=true,autoKey=false;
 float gateDb=-50,threshold=-18,ratio=3,essAmount=.5f,wet=.18f,tuneStrength=.8f;
 int key=0;bool minor=false;float envelope=0,low=0,gateGain=0;std::array<float,4096> pitchHistory{},shift{};unsigned historyPos=0,shiftPos=0,analysisCount=0;double phase=0,pitchRatio=1;std::array<float,12> chroma{};std::atomic<int> detectedKey{0};
 std::array<std::vector<float>,4> delays;std::array<unsigned,4> delayPos{};
 void prepare(double rate){sr=rate;for(int i=0;i<4;++i){delays[i].assign(static_cast<size_t>(sr*(.0297+i*.0089)),0);delayPos[i]=0;}envelope=low=gateGain=0;pitchHistory.fill(0);shift.fill(0);historyPos=shiftPos=analysisCount=0;phase=0;pitchRatio=1;chroma.fill(0);for(auto& b:eq)b.z1=b.z2=0;}
 void analyse(){int lo=std::max(1,int(sr/1000)),hi=std::min(1024,int(sr/65));double best=0;int lag=0;double energy=0;for(int i=0;i<1024;++i){float x=pitchHistory[(historyPos-1-i)&4095];energy+=x*x;}if(energy<.002)return;
  for(int l=lo;l<=hi;++l){double cross=0,e2=0;for(int i=0;i<1024;i+=4){float a=pitchHistory[(historyPos-1-i)&4095],b=pitchHistory[(historyPos-1-i-l)&4095];cross+=a*b;e2+=b*b;}double corr=cross/std::sqrt(std::max(1e-12,energy*.25*e2));if(corr>best){best=corr;lag=l;}}if(best<.75||lag==0)return;
  double note=69+12*std::log2(sr/lag/440.);int pc=(int(std::round(note))%12+12)%12;for(auto& c:chroma)c*=.998f;chroma[pc]+=1;
  static constexpr int major[]{0,2,4,5,7,9,11},min[]{0,2,3,5,7,8,10};int root=key;bool isMinor=minor;
  if(autoKey){float score=-1;int winner=0;for(int m=0;m<2;++m)for(int r=0;r<12;++r){float s=0;for(int d=0;d<7;++d)s+=chroma[(r+(m?min[d]:major[d]))%12];s+=chroma[r]*.2f;if(s>score){score=s;winner=r+m*12;}}detectedKey=winner;root=winner%12;isMinor=winner>=12;}
  double distance=100,target=note;for(int n=int(std::floor(note))-12;n<=int(std::ceil(note))+12;++n)for(int d=0;d<7;++d)if((n%12+12)%12==(root+(isMinor?min[d]:major[d]))%12&&std::abs(note-n)<distance){distance=std::abs(note-n);target=n;}
  double desired=std::pow(2.,(target-note)*tuneStrength/12.);pitchRatio=.85*pitchRatio+.15*desired;
 }
 float shifted(float x){shift[shiftPos&4095]=x;double len=1024;phase+= (1-pitchRatio)/len;phase-=std::floor(phase);auto read=[&](double ph){double position=double(shiftPos)-256-ph*len;int i=int(std::floor(position));float frac=float(position-i);return shift[i&4095]*(1-frac)+shift[(i+1)&4095]*frac;};double second=phase+.5;second-=std::floor(second);float w=float(.5-.5*std::cos(phase*2*3.141592653589793));float y=read(phase)*w+read(second)*(1-w);++shiftPos;return y;}
 float process(float x){float absolute=std::abs(x),coeff=absolute>envelope?.02f:.001f;envelope+=coeff*(absolute-envelope);if(gate){float target=envelope>std::pow(10.f,gateDb/20)?1.f:0.f;gateGain+=.002f*(target-gateGain);x*=gateGain;}if(compressor){float db=20*std::log10(std::max(envelope,1e-8f));if(db>threshold)x*=std::pow(10.f,((threshold+(db-threshold)/ratio)-db)/20.f);}if(equalizer)for(auto& b:eq)x=b.process(x);if(deesser){low+=float(1-std::exp(-2*3.141592653589793*4500/sr))*(x-low);float high=x-low;x-=high*std::min(.85f,std::abs(high)*8*essAmount);}pitchHistory[historyPos++&4095]=x;if(tune||autoKey){if(++analysisCount>=1024){analysisCount=0;analyse();}}if(tune)x=shifted(x);if(reverb){float sum=0;for(int i=0;i<4;++i){auto& d=delays[i];auto& pos=delayPos[i];float old=d[pos];d[pos]=x+old*.72f;sum+=old;pos=(pos+1)%static_cast<unsigned>(d.size());}x=x*(1-wet)+sum*.25f*wet;}return std::isfinite(x)?x:0;}
};
