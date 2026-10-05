#include "../native/ambience.h"
#include "../native/master.h"
#include "../native/dsp.h"
#include <iostream>
#include <stdexcept>
void check(bool v,const char* m){if(!v)throw std::runtime_error(m);}
int main(){try{for(double sr:{44100.,48000.,96000.}){
 StereoAmbience a;a.reverb=false;a.prepare(sr);auto identity=a.process(.2f,-.13f);check(identity[0]==.2f&&identity[1]==-.13f,"Bypassed stereo ambience changed dry audio");
 a.delay=true;a.pingPong=true;a.duck=0;a.feedback=.5f;a.delayWet=.5f;a.delayMs=100;a.configure();double left=0,right=0;unsigned delay=unsigned(sr*.1);
 for(unsigned i=0;i<delay*3;++i){auto out=a.process(i==0?.2f:0,i==0?.2f:0);if(i>=delay&&i<delay*2)left+=std::abs(out[0]);if(i>=delay*2)right+=std::abs(out[1]);check(std::isfinite(out[0])&&std::isfinite(out[1]),"Nonfinite delay");}check(left>.001&&right>.001,"Ping pong repeats did not alternate channels");
 // A muted insert must also mute its downstream tail; an amplified insert must feed the tail at that gain.
 StereoAmbience silent,normal,boost;for(auto* p:{&silent,&normal,&boost}){p->reverb=false;p->delay=true;p->pingPong=false;p->duck=0;p->feedback=0;p->delayMs=100;p->prepare(sr);}double ordinary=0,louder=0;for(unsigned i=0;i<delay*2;++i){float raw=i==0?.2f:0;auto off=silent.process(raw*0,raw*0);auto n=normal.process(raw,raw),b=boost.process(raw*2,raw*2);check(off[0]==0&&off[1]==0,"Tail was fed before a muted VST");if(i>=delay){ordinary+=n[0]*n[0];louder+=b[0]*b[0];}}check(ordinary>0&&std::abs(louder/ordinary-4)<.001,"Post-VST delay did not follow insert gain");
 StereoAmbience room;room.prepare(sr);double early=0,late=0,stereo=0;for(int i=0;i<int(sr*7);++i){auto v=room.process(i==0?.5f:0,i==0?.5f:0);check(std::isfinite(v[0])&&std::abs(v[0])<2,"Reverb unstable");if(i>0&&i<sr*2){early+=v[0]*v[0]+v[1]*v[1];stereo+=std::abs(v[0]-v[1]);}if(i>sr*6)late+=v[0]*v[0]+v[1]*v[1];}check(early>1e-7&&stereo>.001&&late<early*.01,"Reverb tail not stereo/decaying");
 MasterProcessor m;m.prepare(sr);auto off=m.process(.2f,-.1f);check(off[0]==.2f&&off[1]==-.1f,"Master bypass changed audio");m.enabled=true;m.lowDb=6;m.highDb=6;m.loudDb=12;m.width=1.5f;m.thick=1;m.glue=1;m.configure();for(int i=0;i<int(sr);++i){auto out=m.process(2*std::sin(float(i*.1)),2*std::cos(float(i*.17)));check(std::isfinite(out[0])&&std::abs(out[0])<=1&&std::abs(out[1])<=1,"Master clip stage exceeded bounded output");}
 MasterProcessor mono;mono.enabled=true;mono.width=1.5f;mono.prepare(sr);for(int i=0;i<1000;++i){auto v=mono.process(.2f*std::sin(i*.1f),.2f*std::sin(i*.1f));check(v[0]==v[1],"Master width broke centered mono");}
 }std::cout<<"Post FX tests passed: stereo bypass, ping pong, post-insert gain/mute, stereo reverb decay, master bypass/mono/bounds at 44.1/48/96 kHz\n";return 0;}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
