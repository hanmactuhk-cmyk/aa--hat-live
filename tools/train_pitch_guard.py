"""Train the small periodic-pitch confidence network shipped in native/pitch_guard.h.
Synthetic harmonic/noise windows, not a pretrained speech or singing model.
Reproduction: Python 3 + numpy; run from the repository root.
"""
import numpy as np
from pathlib import Path
rng=np.random.default_rng(1706)
x=[];y=[]
for i in range(2400):
 voiced=i%2==0;f=rng.uniform(70,950);t=np.arange(800)/12000
 if voiced:
  a=np.sin(2*np.pi*f*t+rng.uniform(0,6.28))
  for h in range(2,6):a+=rng.uniform(0,.6)/h*np.sin(2*np.pi*f*h*t+rng.uniform(0,6.28))
  a+=rng.normal(0,rng.uniform(0,.25),len(t))
 else:
  a=rng.normal(0,1,len(t))
  if i%3==0:a=np.convolve(a,np.ones(5)/5,mode='same')
  if i%5==0:a+=rng.uniform(.1,1)*np.sin(2*np.pi*f*t)
 a*=10**rng.uniform(-2.4,-.4)
 d=np.array([np.sum((a[:512]-a[k:k+512])**2) for k in range(1,185)])
 diff=d*np.arange(1,185)/np.maximum(np.cumsum(d),1e-10);lag=12+int(np.argmin(diff[11:]));confidence=1-diff[lag-1]
 rms=np.sqrt(np.mean(a[:512]**2));z=np.mean(a[:511]*a[1:512]<0);hz=12000/lag
 x.append([confidence,np.clip((np.log10(rms+1e-9)+3)/3,0,1),np.clip(z*12000/(2*hz)/4,0,1),hz/1000]);y.append(float(voiced))
x=np.array(x);y=np.array(y)[:,None];perm=rng.permutation(len(x));x=x[perm];y=y[perm];train=2000
w=rng.normal(0,.3,(4,8));b=np.zeros((1,8));v=rng.normal(0,.3,(8,1));c=np.zeros((1,1))
for step in range(2200):
 h=np.tanh(x[:train]@w+b);o=1/(1+np.exp(-np.clip(h@v+c,-20,20)));d=(o-y[:train])/train
 dw=x[:train].T@((d@v.T)*(1-h*h));db=np.sum((d@v.T)*(1-h*h),axis=0,keepdims=True);dv=h.T@d;dc=np.sum(d,axis=0,keepdims=True)
 w-=.16*dw;b-=.16*db;v-=.16*dv;c-=.16*dc
out=1/(1+np.exp(-(np.tanh(x[train:]@w+b)@v+c)))
print('Held-out synthetic accuracy:',np.mean((out>.5)==y[train:]))
fmt=lambda a:','.join(f'{q:.9f}f' for q in a.flatten())
header='''#pragma once
#include <cmath>
#include <algorithm>
// 4 -> 8 tanh -> sigmoid. Synthetic periodicity classifier, not a speech model.
// Offline trained by tools/train_pitch_guard.py; zero allocations, no cloud.
inline float neuralPitchConfidence(float confidence,float rms,float zcr,float hz){
'''
header+=f'const float w[32]={{{fmt(w)}}},b[8]={{{fmt(b)}}},v[8]={{{fmt(v)}}};float sum={fmt(c)};\n'
header+='float in[4]={confidence,std::clamp((std::log10(rms+1e-9f)+3)/3,0.f,1.f),std::clamp(zcr*12000/(2*hz)/4,0.f,1.f),hz/1000};for(int j=0;j<8;++j){float s=b[j];for(int i=0;i<4;++i)s+=in[i]*w[i*8+j];sum+=std::tanh(s)*v[j];}return 1/(1+std::exp(-sum));}\n'
Path('native/pitch_guard.h').write_text(header)
