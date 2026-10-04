#pragma once
#include <cmath>
#include <algorithm>
// 4 -> 8 tanh -> sigmoid. Synthetic periodicity classifier, not a speech model.
// Offline trained by tools/train_pitch_guard.py; zero allocations, no cloud.
inline float neuralPitchConfidence(float confidence,float rms,float zcr,float hz){
const float w[32]={0.534207248f,-1.989226075f,2.543871912f,0.831837870f,1.419577777f,0.369511279f,-0.649754800f,2.084660366f,0.104464311f,-0.030983166f,-0.414025564f,0.134620711f,-0.135969512f,-0.328791896f,-0.073676717f,-0.363379226f,-0.026743218f,0.948481434f,-0.571483379f,-0.186386067f,-0.270468854f,0.057656690f,0.190917196f,-0.401481964f,-0.147431433f,0.179640530f,-0.388789122f,0.336537053f,-0.011347165f,0.240623797f,0.101556363f,-1.177013487f},b[8]={-0.165582391f,0.472635186f,-1.116632512f,-0.292332276f,-0.594697333f,-0.050087079f,0.141982722f,-0.803009029f},v[8]={0.589802322f,-2.312270896f,3.204476799f,0.866904825f,1.579088586f,0.251029333f,-0.658700940f,2.669880274f};float sum=-0.302138484f;
float in[4]={confidence,std::clamp((std::log10(rms+1e-9f)+3)/3,0.f,1.f),std::clamp(zcr*12000/(2*hz)/4,0.f,1.f),hz/1000};for(int j=0;j<8;++j){float s=b[j];for(int i=0;i<4;++i)s+=in[i]*w[i*8+j];sum+=std::tanh(s)*v[j];}return 1/(1+std::exp(-sum));}
