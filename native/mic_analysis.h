#pragma once
#include <algorithm>
#include <array>
#include <cmath>
// Audio-thread-owned statistics. No allocations. Publish only after collection ends.
struct MicAnalysis {
 unsigned long long samples=0,voiced=0;double sr=48000,sum=0,lowEnergy=0,highEnergy=0,peak=0,lp=0,hp=0,lowCoeff=.03,highCoeff=.5;
 std::array<unsigned long long,91> histogram{};
 void reset(double rate){*this=MicAnalysis{};sr=rate;lowCoeff=1-std::exp(-2*3.141592653589793*250/sr);highCoeff=1-std::exp(-2*3.141592653589793*5500/sr);}
 void feed(float value){double x=std::isfinite(value)?value:0,a=std::abs(x);++samples;peak=std::max(peak,a);int db=std::clamp(int(20*std::log10(std::max(a,1e-9))),-90,0);++histogram[db+90];lp+=lowCoeff*(x-lp);hp+=highCoeff*(x-hp);if(a>.003){++voiced;sum+=x*x;lowEnergy+=lp*lp;highEnergy+=(x-hp)*(x-hp);}}
 struct Result {bool valid=false;float gateDb=-50,compressorDb=-18,ratio=3,essAmount=.5f,reverbWet=.16f,micVolume=.8f;std::array<float,13> eq{};};
 Result result()const{Result r;if(voiced<sr*.2||sum<.0001||peak<.008)return r;r.valid=true;double rms=std::sqrt(sum/voiced),db=20*std::log10(rms);unsigned long long cumulative=0;int floor=-70;for(int i=0;i<=90;++i){cumulative+=histogram[i];if(cumulative>=samples/10){floor=i-90;break;}}r.gateDb=float(std::clamp(double(floor+8),-65.,std::min(-25.,db-12)));r.compressorDb=float(std::clamp(db-4,-30.,-10.));r.reverbWet=rms>.15?.12f:.18f;r.ratio=peak/rms>5?3.5f:2.5f;r.essAmount=float(std::clamp(highEnergy/sum*4,.15,.8));r.micVolume=float(std::clamp(.20/rms,.25,1.25));r.eq[0]=-6;r.eq[1]=-4;r.eq[2]=-2;r.eq[4]=lowEnergy/sum>.25?-2.5f:-.5f;r.eq[8]=1;r.eq[9]=1.5f;r.eq[11]=highEnergy/sum>.08?-2:1;return r;}
};
