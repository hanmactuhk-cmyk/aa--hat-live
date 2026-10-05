#pragma once
#include <array>
#include "pitch_guard.h"
#include <atomic>
#include <cmath>
#include <algorithm>
#include <vector>
// Bounded-memory vocal DSP. No allocations or IPC in process().
struct Biquad {
 float b0=1,b1=0,b2=0,a1=0,a2=0,z1=0,z2=0;
 void peak(double sr,double hz,double db){double A=std::pow(10.,db/40.),w=2*3.141592653589793*std::min(hz,sr*.45)/sr,alpha=std::sin(w)/2;double a0=1+alpha/A;b0=float((1+alpha*A)/a0);b1=float(-2*std::cos(w)/a0);b2=float((1-alpha*A)/a0);a1=b1;a2=float((1-alpha/A)/a0);}
 void bandpass(double sr,double hz){double w=2*3.141592653589793*std::min(hz,sr*.45)/sr,alpha=std::sin(w)/2,a0=1+alpha;b0=float(alpha/a0);b1=0;b2=-b0;a1=float(-2*std::cos(w)/a0);a2=float((1-alpha)/a0);z1=z2=0;}
 float process(float x){float y=b0*x+z1;z1=b1*x-a1*y+z2;z2=b2*x-a2*y;return y;}
};
struct VocalDSP {
 double sr=48000;std::array<Biquad,13> eq{};
 static constexpr std::array<double,13> hz{40,63,100,160,250,400,630,1000,1600,2500,4000,6300,10000};
 bool aiAssist=true;bool highpass=false,warmth=false,air=false;float highpassHz=80,warmthAmount=.2f,airAmount=.15f,reverbDecay=1.8f,reverbDamping=.55f,reverbPre=25;float hpLow=0,airLow=0;std::array<float,4> damp{};std::vector<float> predelay;unsigned prePos=0;
 bool gate=true,compressor=true,equalizer=true,deesser=true,tune=false,reverb=true,autoKey=true,shortEcho=false,longEcho=false;
 float gateDb=-50,threshold=-18,ratio=3,essAmount=.5f,wet=.18f,tuneStrength=.8f,tuneSpeed=35;bool chromatic=false;float shortWet=.15f,longWet=.18f,shortMs=120,longMs=380,shortFeedback=.2f,longFeedback=.4f;
 int key=0;bool minor=false;float envelope=0,low=0,gateGain=0;std::array<float,8192> pitchHistory{};std::array<float,4096> shift{};unsigned historyPos=0,shiftPos=0,analysisCount=0;double phase=0,pitchRatio=1;std::array<float,12> chroma{};std::atomic<int> detectedKey{0};std::atomic<float> detectedHz{0},targetHz{0},pitchConfidence{0};std::array<double,256> difference{};int keyCandidate=-1,keyHold=0;unsigned keyFrames=0;std::atomic<float> keyConfidence{0};std::atomic<bool> keyReady{false};std::array<Biquad,13> spectrum{};std::array<float,13> spectrumPeaks{};std::array<std::atomic<float>,13> eqLevels{};std::array<std::vector<float>,2> echoes;unsigned echoPosition=0;
 std::array<std::vector<float>,4> delays;std::array<unsigned,4> delayPos{};
 void resetKey(){chroma.fill(0);keyCandidate=-1;keyHold=0;keyFrames=0;keyConfidence=0;keyReady=false;}
 void publishSpectrum(){for(unsigned i=0;i<13;++i){eqLevels[i]=spectrumPeaks[i];spectrumPeaks[i]=0;}}
 void prepare(double rate){sr=rate;predelay.assign(size_t(sr*.121)+2,0);prePos=0;hpLow=airLow=0;damp.fill(0);for(auto& line:echoes)line.assign(static_cast<size_t>(sr*1.1)+2,0);echoPosition=0;for(unsigned i=0;i<13;++i){spectrum[i].bandpass(sr,hz[i]);spectrumPeaks[i]=0;eqLevels[i]=0;}resetKey();for(int i=0;i<4;++i){delays[i].assign(static_cast<size_t>(sr*(.0311+i*.0117)),0);delayPos[i]=0;}envelope=low=gateGain=0;pitchHistory.fill(0);shift.fill(0);historyPos=shiftPos=analysisCount=0;phase=0;pitchRatio=1;chroma.fill(0);detectedHz=targetHz=pitchConfidence=0;detectedKey=key;keyCandidate=-1;keyHold=0;for(auto& b:eq)b.z1=b.z2=0;}
 void analyse(){
  // YIN difference on a downsampled window; costs are bounded independently of UI.
  const int stride=std::max(1,int(sr/12000)),window=512,maxLag=std::min(250,int(sr/stride/65)),minLag=std::max(2,int(sr/stride/1000));
  double energy=0;for(int i=0;i<window;++i){double x=pitchHistory[(historyPos-1-i*stride)&8191];energy+=x*x;}
  if(energy/window<.00001){detectedHz=targetHz=pitchConfidence=0;pitchRatio+=(1-pitchRatio)*.2;return;}
  difference[0]=1;double running=0;int lag=0;for(int l=1;l<=maxLag;++l){double sum=0;for(int i=0;i<window;++i){double delta=pitchHistory[(historyPos-1-i*stride)&8191]-pitchHistory[(historyPos-1-(i+l)*stride)&8191];sum+=delta*delta;}running+=sum;difference[l]=running>0?sum*l/running:1;}
  for(int l=minLag;l<maxLag-1;++l)if(difference[l]<.15){while(l<maxLag-1&&difference[l+1]<difference[l])++l;lag=l;break;}
  if(!lag){detectedHz=targetHz=pitchConfidence=0;pitchRatio+=(1-pitchRatio)*.2;return;}
  double refined=lag;double denom=difference[lag-1]-2*difference[lag]+difference[lag+1];if(std::abs(denom)>1e-12)refined+=std::clamp(.5*(difference[lag-1]-difference[lag+1])/denom,-.5,.5);
  double frequency=sr/stride/refined;if(aiAssist){int crossings=0;for(int i=1;i<window;++i)if(pitchHistory[(historyPos-1-i*stride)&8191]*pitchHistory[(historyPos-1-(i-1)*stride)&8191]<0)++crossings;float zcr=float(crossings)/(window-1)*float(sr/stride/12000);float probability=neuralPitchConfidence(float(1-difference[lag]),float(std::sqrt(energy/window)),zcr,float(frequency));if(probability<.55f){detectedHz=targetHz=pitchConfidence=0;pitchRatio+=(1-pitchRatio)*.2;return;}}double note=69+12*std::log2(frequency/440.);detectedHz=float(frequency);pitchConfidence=float(1-difference[lag]);
  int pc=(int(std::round(note))%12+12)%12;for(auto& c:chroma)c*=.995f;chroma[pc]+=float(1-difference[lag]);
  static constexpr int major[]{0,2,4,5,7,9,11},min[]{0,2,3,5,7,8,10};int root=key;bool isMinor=minor;
  if(autoKey){
   static constexpr float profiles[2][12]={{6.35f,2.23f,3.48f,2.33f,4.38f,4.09f,2.52f,5.19f,2.39f,3.66f,2.29f,2.88f},{6.33f,2.68f,3.52f,5.38f,2.6f,3.53f,2.54f,4.75f,3.98f,2.69f,3.34f,3.17f}};
   ++keyFrames;float best=-2,second=-2,mean=0,maxCount=0;for(auto x:chroma){mean+=x/12;maxCount=std::max(maxCount,x);}int distinct=0;for(auto x:chroma)if(x>maxCount*.08f)++distinct;int winner=0;
   for(int m=0;m<2;++m)for(int r=0;r<12;++r){float pm=0;for(float x:profiles[m])pm+=x/12;float xy=0,xx=0,yy=0;for(int n=0;n<12;++n){float x=chroma[(r+n)%12]-mean,y=profiles[m][n]-pm;xy+=x*y;xx+=x*x;yy+=y*y;}float score=xy/std::sqrt(std::max(1e-9f,xx*yy));if(score>best){second=best;best=score;winner=r+m*12;}else second=std::max(second,score);}
   keyConfidence=std::clamp(best,0.f,1.f);if(distinct>=4&&keyFrames>=100&&best>.45f&&best-second>.025f){if(winner==keyCandidate)++keyHold;else{keyCandidate=winner;keyHold=0;}if(keyHold>=15){detectedKey=winner;keyReady=true;}}else keyHold=0;
   if(keyReady){root=detectedKey.load()%12;isMinor=detectedKey.load()>=12;}
  }
  double target=std::round(note),distance=100;if(!chromatic&&(!autoKey||keyReady.load())){target=note;for(int n=int(std::floor(note))-12;n<=int(std::ceil(note))+12;++n)for(int d=0;d<7;++d)if((n%12+12)%12==(root+(isMinor?min[d]:major[d]))%12&&std::abs(note-n)<distance){distance=std::abs(note-n);target=n;}}
  targetHz=float(440*std::pow(2.,(target-69)/12.));double desired=std::pow(2.,(target-note)*tuneStrength/12.);double alpha=1-std::exp(-20./std::max(1.f,tuneSpeed));pitchRatio+=(desired-pitchRatio)*alpha;
 }
 float shifted(float x){shift[shiftPos&4095]=x;double len=1024;phase+= (1-pitchRatio)/len;phase-=std::floor(phase);auto read=[&](double ph){double position=double(shiftPos&4095)-256-ph*len;int i=int(std::floor(position));float frac=float(position-i);return shift[i&4095]*(1-frac)+shift[(i+1)&4095]*frac;};double second=phase+.5;second-=std::floor(second);float w=float(.5-.5*std::cos(phase*2*3.141592653589793));float y=read(phase)*w+read(second)*(1-w);++shiftPos;return y;}
 float processDry(float x){if(highpass){hpLow+=float(1-std::exp(-2*3.141592653589793*highpassHz/sr))*(x-hpLow);x-=hpLow;}float absolute=std::abs(x),coeff=absolute>envelope?.02f:.001f;envelope+=coeff*(absolute-envelope);if(gate){float target=envelope>std::pow(10.f,gateDb/20)?1.f:0.f;gateGain+=.002f*(target-gateGain);x*=gateGain;}if(compressor){float db=20*std::log10(std::max(envelope,1e-8f));if(db>threshold)x*=std::pow(10.f,((threshold+(db-threshold)/ratio)-db)/20.f);}if(equalizer)for(auto& b:eq)x=b.process(x);for(unsigned i=0;i<13;++i)spectrumPeaks[i]=std::max(spectrumPeaks[i],std::abs(spectrum[i].process(x)));if(deesser){low+=float(1-std::exp(-2*3.141592653589793*4500/sr))*(x-low);float high=x-low;x-=high*std::min(.85f,std::abs(high)*8*essAmount);}pitchHistory[historyPos++&8191]=x;if(tune||autoKey){if(++analysisCount>=unsigned(sr*.02)){analysisCount=0;analyse();}}if(tune)x=shifted(x);if(warmth){float drive=1+warmthAmount*4;float shaped=std::tanh(x*drive)/drive;x+=(shaped-x)*warmthAmount;}if(air){airLow+=float(1-std::exp(-2*3.141592653589793*6000/sr))*(x-airLow);x+=(x-airLow)*airAmount;}return std::isfinite(x)?x:0;}
 float process(float x){return processAmbience(processDry(x));}
 float processAmbience(float x){if(reverb&&!predelay.empty()){unsigned n=unsigned(predelay.size());unsigned offset=unsigned(sr*reverbPre/1000);predelay[prePos]=x;float feed=predelay[(prePos+n-offset)%n];prePos=(prePos+1)%n;float sum=0;for(int i=0;i<4;++i){float old=delays[i][delayPos[i]];damp[i]+=(1-reverbDamping*.92f)*(old-damp[i]);sum+=damp[i];}for(int i=0;i<4;++i){auto& d=delays[i];auto& pos=delayPos[i];float feedback=float(std::pow(.001,double(d.size())/sr/std::max(.25f,reverbDecay)));d[pos]=feed*.3f+(sum*.5f-damp[i])*feedback;pos=(pos+1)%unsigned(d.size());}x+=sum*.5f*wet;}float shortSample=0,longSample=0;if(!echoes[0].empty()){const unsigned size=unsigned(echoes[0].size()),pos=echoPosition%size;auto read=[&](int i,float ms){auto delay=unsigned(std::clamp(sr*ms/1000,1.,double(size-1)));return echoes[i][(pos+size-delay)%size];};shortSample=read(0,shortMs);longSample=read(1,longMs);echoes[0][pos]=shortEcho?x+std::tanh(shortSample)*shortFeedback:0;echoes[1][pos]=longEcho?x+std::tanh(longSample)*longFeedback:0;echoPosition=(pos+1)%size;}if(shortEcho)x+=shortSample*shortWet;if(longEcho)x+=longSample*longWet;return std::isfinite(x)?x:0;}
};
