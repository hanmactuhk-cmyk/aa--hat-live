#include <juce_audio_utils/juce_audio_utils.h>
#include "../ambience.h"
class HNDelay final:public juce::AudioProcessor {
 StereoAmbience effect;std::array<juce::AudioParameterFloat*,10> p{};
public:
 HNDelay():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){
  const char* names[]{"Enabled","Wet","Time ms","BPM","Sync","Division","Feedback","Ducking","Tone","Ping Pong"};const float lo[]{0,0,60,40,0,.25f,0,0,0,0},hi[]{1,.8f,1500,240,1,2,.8f,1,1,1},def[]{0,.2f,380,120,0,1,.32f,.45f,.55f,1};for(int i=0;i<10;++i){p[i]=new juce::AudioParameterFloat(juce::ParameterID("delay"+juce::String(i),1),names[i],juce::NormalisableRange<float>(lo[i],hi[i]),def[i]);addParameter(p[i]);}effect.reverb=false;
 }
 const juce::String getName()const override{return "HNStudio Delay";}bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return 16;}int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return {};}void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{effect.prepare(sr);}void releaseResources()override{}void reset()override{effect.prepare(getSampleRate()>0?getSampleRate():48000);}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{juce::ScopedNoDenormals guard;effect.delay=*p[0]>=.5f;effect.delayWet=*p[1];effect.delayMs=*p[2];effect.bpm=*p[3];effect.sync=*p[4]>=.5f;effect.division=*p[5];effect.feedback=*p[6];effect.duck=*p[7];effect.tone=*p[8];effect.pingPong=*p[9]>=.5f;effect.configure();if(b.getNumChannels()<2)return;for(int i=0;i<b.getNumSamples();++i){auto x=effect.process(b.getSample(0,i),b.getSample(1,i));b.setSample(0,i,x[0]);b.setSample(1,i,x[1]);}}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override{return new juce::GenericAudioProcessorEditor(*this);}
 void getStateInformation(juce::MemoryBlock& data)override{juce::MemoryOutputStream stream(data,false);for(auto* x:p)stream.writeFloat(x->get());}
 void setStateInformation(const void* data,int size)override{if(size!=int(p.size()*sizeof(float)))return;juce::MemoryInputStream stream(data,size,false);for(auto* x:p)x->setValueNotifyingHost(x->convertTo0to1(stream.readFloat()));}
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new HNDelay;}
