#include "routing.h"
#include <iostream>

int main(){
 Routing route;route.asio=true;route.mic=1;route.musicLeft=5;route.musicRight=7;route.monitorLeft=4;route.monitorRight=6;route.sendEnabled=true;route.sendLeft=1;route.sendRight=3;
 route.validate(8,8);
 float samples[8][2]{};const float* inputs[8];float* outputs[8];
 for(int i=0;i<8;++i){inputs[i]=samples[i];outputs[i]=samples[i];}
 samples[1][0]=.1f;samples[5][0]=.2f;samples[7][0]=.3f;
 auto music=route.music(inputs,8,0);
 if(Routing::read(inputs,8,route.mic,0)!=.1f||music[0]!=.2f||music[1]!=.3f)return 1;
 for(auto& channel:samples)channel[0]=0;
 route.write(outputs,8,0,.25f,.5f);
 for(int i=0;i<8;++i){float expected=i==4||i==1?.25f:i==6||i==3?.5f:0.f;if(samples[i][0]!=expected)return 2;}
 route.sendEnabled=false;route.write(outputs,8,1,.2f,.4f);
 if(samples[1][1]!=0||samples[3][1]!=0||samples[4][1]!=.2f||samples[6][1]!=.4f)return 3;
 if(Routing::read(inputs,1,4,0)!=0||route.requiredInputs()!=8||route.requiredOutputs()!=7)return 4;
 route.musicLeft=route.mic;try{route.validate(8,8);return 5;}catch(const std::runtime_error&){}
 route.musicLeft=5;route.sendEnabled=true;route.sendLeft=route.monitorLeft;try{route.validate(8,8);return 6;}catch(const std::runtime_error&){}
 std::cout<<"Routing self-test passed: separate MIC/MUSIC, sparse port indices, two Master pairs and overlap rejection\n";
}
