#include <juce_audio_utils/juce_audio_utils.h>
#include "../master.h"
class HNMaster final:public juce::AudioProcessor {
 MasterProcessor effect;std::array<juce::AudioParameterFloat*,8> p{};
public:
 HNMaster():AudioProcessor(BusesProperties().withInput("Input",juce::AudioChannelSet::stereo(),true).withOutput("Output",juce::AudioChannelSet::stereo(),true)){
 const char* names[]{"Enabled","Low dB","Mid dB","High dB","Width","Thick","Glue","Loudness dB"};const float lo[]{0,-6,-6,-6,0,0,0,0},hi[]{1,6,6,6,1.5f,1,1,12},def[]{0,0,0,0,1,.12f,.2f,0};for(int i=0;i<8;++i){p[i]=new juce::AudioParameterFloat(juce::ParameterID("master"+juce::String(i),1),names[i],juce::NormalisableRange<float>(lo[i],hi[i]),def[i]);addParameter(p[i]);}}
 const juce::String getName()const override{return "HNStudio Master";}bool acceptsMidi()const override{return false;}bool producesMidi()const override{return false;}double getTailLengthSeconds()const override{return 0;}int getNumPrograms()override{return 1;}int getCurrentProgram()override{return 0;}void setCurrentProgram(int)override{}const juce::String getProgramName(int)override{return {};}void changeProgramName(int,const juce::String&)override{}
 bool isBusesLayoutSupported(const BusesLayout& l)const override{return l.getMainInputChannelSet()==juce::AudioChannelSet::stereo()&&l.getMainOutputChannelSet()==juce::AudioChannelSet::stereo();}
 void prepareToPlay(double sr,int)override{effect.prepare(sr);}void releaseResources()override{}void reset()override{effect.prepare(getSampleRate()>0?getSampleRate():48000);}
 void processBlock(juce::AudioBuffer<float>& b,juce::MidiBuffer&)override{juce::ScopedNoDenormals guard;effect.enabled=*p[0]>=.5f;effect.lowDb=*p[1];effect.midDb=*p[2];effect.highDb=*p[3];effect.width=*p[4];effect.thick=*p[5];effect.glue=*p[6];effect.loudDb=*p[7];effect.configure();if(b.getNumChannels()<2)return;for(int i=0;i<b.getNumSamples();++i){auto x=effect.process(b.getSample(0,i),b.getSample(1,i));b.setSample(0,i,x[0]);b.setSample(1,i,x[1]);}}
 bool hasEditor()const override{return true;}juce::AudioProcessorEditor* createEditor()override{return new juce::GenericAudioProcessorEditor(*this);}
 void getStateInformation(juce::MemoryBlock& data)override{juce::MemoryOutputStream stream(data,false);for(auto* x:p)stream.writeFloat(x->get());}
 void setStateInformation(const void* data,int size)override{if(size!=int(p.size()*sizeof(float)))return;juce::MemoryInputStream stream(data,size,false);for(auto* x:p)x->setValueNotifyingHost(x->convertTo0to1(stream.readFloat()));}
};
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter(){return new HNMaster;}
