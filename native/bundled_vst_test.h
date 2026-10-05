#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include <iostream>
#include <stdexcept>
inline int runBundledVstTest(const char* delayPath,const char* masterPath){try{
 juce::AudioPluginFormatManager manager;manager.addFormat(new juce::VST3PluginFormat);
 auto load=[&](const char* path){juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,juce::File(path).getFullPathName());if(found.isEmpty())throw std::runtime_error(std::string("VST3 scan failed: ")+path);juce::String error;auto plugin=manager.createPluginInstance(*found[0],48000,256,error);if(!plugin)throw std::runtime_error(error.toStdString());plugin->setRateAndBufferSizeDetails(48000,256);plugin->prepareToPlay(48000,256);return plugin;};
 auto delay=load(delayPath),master=load(masterPath);juce::AudioBuffer<float> b(2,256);juce::MidiBuffer midi;
 b.clear();b.setSample(0,0,.2f);b.setSample(1,0,-.1f);delay->processBlock(b,midi);if(b.getSample(0,0)!=.2f||b.getSample(1,0)!=-.1f)throw std::runtime_error("Delay VST bypass not transparent");
 auto set=[&](juce::AudioPluginInstance& plugin,int index,float value){plugin.getParameters()[index]->setValueNotifyingHost(value);};
 set(*delay,0,1);set(*delay,1,1);set(*delay,2,0);set(*delay,6,0);set(*delay,7,0);set(*delay,9,0);delay->reset();float echoPeak=0;for(int block=0;block<40;++block){b.clear();if(block==0){b.setSample(0,0,.2f);b.setSample(1,0,.2f);}delay->processBlock(b,midi);if(block>=11)echoPeak=std::max(echoPeak,b.getMagnitude(0,256));}if(echoPeak<.001f)throw std::runtime_error("Bundled Delay VST did not produce real delayed samples");
 juce::MemoryBlock saved;delay->getStateInformation(saved);set(*delay,1,0);delay->setStateInformation(saved.getData(),int(saved.getSize()));if(std::abs(delay->getParameters()[1]->getValue()-1)>1e-6)throw std::runtime_error("Delay VST state failed to restore");
 b.clear();b.setSample(0,0,.2f);b.setSample(1,0,-.1f);master->processBlock(b,midi);if(b.getSample(0,0)!=.2f||b.getSample(1,0)!=-.1f)throw std::runtime_error("Master VST bypass not transparent");
 set(*master,0,1);set(*master,5,0);set(*master,6,0);set(*master,7,.5f);float masterPeak=0;for(int block=0;block<100;++block){for(int i=0;i<256;++i){float x=.01f*std::sin(float((block*256+i)*6.28318530718*440/48000));b.setSample(0,i,x);b.setSample(1,i,x);}master->processBlock(b,midi);if(block>20)masterPeak=std::max(masterPeak,b.getMagnitude(0,256));}if(masterPeak<.017f||masterPeak>.03f)throw std::runtime_error("Master VST loudness parameter did not change actual audio");
 delay->releaseResources();master->releaseResources();std::cout<<"Bundled VST3 host test passed: both binaries scan/load, stereo bypass, delayed output, parameter changes and state restore\n";return 0;
 }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
