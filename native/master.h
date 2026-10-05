#pragma once
#include <array>
#include <algorithm>
#include <cmath>
// Original bus processor: tone, linked glue dynamics, M/S width and soft clipping.
// Final safety limiter is separate and remains active even when this insert is bypassed.
struct MasterProcessor {
 bool enabled=false;float lowDb=0,midDb=0,highDb=0,width=1,thick=.12f,glue=.2f,loudDb=0;
 double sr=48000;std::array<float,2> low{},top{},dc{};float sideLow=0,envelope=0,compGain=1;
 float aLow=0,aTop=0,aDC=0,aSide=0,attack=0,release=0,gLow=1,gMid=1,gHigh=1,gLoud=1;
 void configure(){gLow=std::pow(10.f,lowDb/20);gMid=std::pow(10.f,midDb/20);gHigh=std::pow(10.f,highDb/20);gLoud=std::pow(10.f,loudDb/20);}
 void prepare(double rate){sr=rate;low.fill(0);top.fill(0);dc.fill(0);sideLow=envelope=0;compGain=1;auto a=[&](double f){return float(1-std::exp(-6.28318530718*f/sr));};aLow=a(180);aTop=a(4000);aDC=a(20);aSide=a(140);attack=float(1-std::exp(-1/(sr*.01)));release=float(1-std::exp(-1/(sr*.15)));configure();}
 std::array<float,2> process(float l,float r){std::array<float,2> x{l,r};if(!enabled)return x;for(int ch=0;ch<2;++ch){dc[ch]+=aDC*(x[ch]-dc[ch]);x[ch]-=dc[ch];low[ch]+=aLow*(x[ch]-low[ch]);top[ch]+=aTop*(x[ch]-top[ch]);x[ch]=low[ch]*gLow+(top[ch]-low[ch])*gMid+(x[ch]-top[ch])*gHigh;float drive=1+thick*3;x[ch]+=(std::tanh(x[ch]*drive)/drive-x[ch])*thick;}
  float mid=(x[0]+x[1])*.5f,side=(x[0]-x[1])*.5f;sideLow+=aSide*(side-sideLow);side=sideLow+(side-sideLow)*width;x={mid+side,mid-side};float peak=std::max(std::abs(x[0]),std::abs(x[1]));envelope+=(peak>envelope?attack:release)*(peak-envelope);float threshold=std::pow(10.f,(-12-glue*12)/20);float desired=envelope>threshold?std::pow(threshold/std::max(envelope,1e-9f),glue*.55f):1;compGain+=(desired<compGain?attack:release)*(desired-compGain);for(auto& v:x){v*=compGain*gLoud;float a=std::abs(v);if(a>.85f)v=std::copysign(.85f+.15f*std::tanh((a-.85f)/.15f),v);if(!std::isfinite(v))v=0;}return x;
 }
};
