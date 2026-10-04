#include <juce_audio_utils/juce_audio_utils.h>
#include <juce_dsp/juce_dsp.h>
#include <nlohmann/json.hpp>
#include <future>
#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <array>
#include "loopback.h"
#include "dsp.h"
#include "routing.h"
using json=nlohmann::json;
std::mutex stdoutMutex;
void send(const json& value){std::lock_guard lock(stdoutMutex);std::cout<<value.dump()<<std::endl;}
class Engine final : public juce::AudioIODeviceCallback,public juce::Timer {
 juce::AudioDeviceManager devices;juce::AudioPluginFormatManager formats;juce::TimeSliceThread disk{"WAV writer"};
 struct Slot{std::unique_ptr<juce::AudioPluginInstance> plugin;bool on=true,bypass=false;std::string path;};
 std::array<Slot,4> slots;juce::AudioBuffer<float> vocal;juce::MidiBuffer midi;Loopback music;VocalDSP dsp;
 std::unique_ptr<juce::AudioFormatWriter::ThreadedWriter> recorder;
 std::atomic<unsigned> recordDrops{0};std::array<std::atomic<float>,3> peaks{};std::array<std::atomic<float>,128> waveform{};unsigned waveIndex=0,waveCount=0;
 std::atomic<bool> attached{false},live{false};std::atomic<int> testSamples{0};float micVolume=.8f,musicVolume=.5f,masterVolume=.7f;float limiterGain=1;double rate=48000,oscillator=0;int block=512;json settings;
 std::atomic<unsigned> underruns{0};bool primed=false;
 Routing routing;
 void detach(){if(attached.exchange(false))devices.removeAudioCallback(this);}
 void attach(){if(!attached.exchange(true))devices.addAudioCallback(this);}
 void open(const json& c){
  detach();music.stop();live=false;recorder.reset();
  auto backend=c.value("backend",std::string("Windows Audio"));
  juce::AudioIODeviceType* selectedType=nullptr;
  for(auto* type:devices.getAvailableDeviceTypes())if(type->getTypeName()==juce::String(backend)){type->scanForDevices();selectedType=type;break;}
  if(!selectedType)throw std::runtime_error("Audio backend unavailable: "+backend);
  Routing next;next.asio=backend=="ASIO";
  next.mic=c.value("micChannel",0);next.musicLeft=c.value("musicLeft",2);next.musicRight=c.value("musicRight",3);
  next.monitorLeft=next.asio?c.value("monitorLeft",0):0;next.monitorRight=next.asio?c.value("monitorRight",1):1;
  next.sendEnabled=next.asio&&c.value("sendEnabled",false);next.sendLeft=c.value("sendLeft",2);next.sendRight=c.value("sendRight",3);
  devices.closeAudioDevice();devices.setCurrentAudioDeviceType(juce::String(backend),true);
  auto setup=devices.getAudioDeviceSetup();
  if(next.asio){auto driver=c.value("driver",std::string());if(driver.empty())throw std::runtime_error("Select an installed ASIO x64 driver first");setup.inputDeviceName=juce::String(driver);setup.outputDeviceName=juce::String(driver);}
  else{setup.inputDeviceName=juce::String(c.value("input",setup.inputDeviceName.toStdString()));setup.outputDeviceName=juce::String(c.value("output",setup.outputDeviceName.toStdString()));}
  setup.sampleRate=c.value("sampleRate",48000.);setup.bufferSize=c.value("bufferSize",512);
  setup.useDefaultInputChannels=false;setup.useDefaultOutputChannels=false;setup.inputChannels.clear();setup.outputChannels.clear();
  setup.inputChannels.setRange(0,next.requiredInputs(),true);setup.outputChannels.setRange(0,next.requiredOutputs(),true);
  auto error=devices.setAudioDeviceSetup(setup,true);if(error.isNotEmpty())throw std::runtime_error("Open audio: "+error.toStdString());
  auto* dev=devices.getCurrentAudioDevice();if(!dev)throw std::runtime_error("No output device");
  try{next.validate(dev->getInputChannelNames().size(),dev->getOutputChannelNames().size());if(dev->getActiveInputChannels().countNumberOfSetBits()!=next.requiredInputs()||dev->getActiveOutputChannels().countNumberOfSetBits()!=next.requiredOutputs())throw std::runtime_error("Driver did not activate the selected port range");}catch(...){devices.closeAudioDevice();throw;}
  routing=next;rate=dev->getCurrentSampleRate();block=dev->getCurrentBufferSizeSamples();vocal.setSize(2,std::max(block,8192));dsp.prepare(rate);applyDSP(settings);limiterGain=1;
  for(auto& s:slots)if(s.plugin){s.plugin->releaseResources();s.plugin->setRateAndBufferSizeDetails(rate,block);s.plugin->prepareToPlay(rate,block);}
  settings["backend"]=backend;settings["input"]=setup.inputDeviceName.toStdString();settings["output"]=setup.outputDeviceName.toStdString();settings["sampleRate"]=rate;settings["bufferSize"]=block;attach();
 }
 void applyDSP(const json& c){
  micVolume=c.value("micVolume",.8f);musicVolume=c.value("musicVolume",.5f);masterVolume=c.value("masterVolume",.7f);
  dsp.gate=c.value("gate",true);dsp.compressor=c.value("compressor",true);dsp.equalizer=c.value("eq",true);dsp.deesser=c.value("deesser",true);dsp.tune=c.value("tune",false);dsp.reverb=c.value("reverb",true);dsp.autoKey=c.value("autoKey",false);
  dsp.gateDb=c.value("gateDb",-50.f);dsp.threshold=c.value("compressorDb",-18.f);dsp.ratio=c.value("ratio",3.f);dsp.essAmount=c.value("essAmount",.5f);dsp.wet=c.value("reverbWet",.18f);dsp.tuneStrength=c.value("tuneStrength",.8f);
  std::string key=c.value("key",std::string("C"));dsp.minor=key.size()>1;dsp.key=key[0]=='D'?2:key[0]=='E'?4:key[0]=='F'?5:0;
  auto gains=c.value("eqGains",std::vector<float>(13,0));for(int i=0;i<13;++i)dsp.eq[i].peak(rate,VocalDSP::hz[i],gains.at(i));
 }
 void validate(const json& c){
  for(auto name:{"gate","compressor","eq","deesser","tune","reverb","autoKey"})if(c.contains(name)&&!c[name].is_boolean())throw std::runtime_error("Invalid effect toggle");
  for(auto name:{"micVolume","musicVolume","masterVolume"})if(c.contains(name)&&(!c[name].is_number()||c[name].get<double>()<0||c[name].get<double>()>2))throw std::runtime_error("Invalid volume");
  auto range=[&](const char* name,double lo,double hi){if(c.contains(name)&&(!c[name].is_number()||c[name].get<double>()<lo||c[name].get<double>()>hi))throw std::runtime_error(std::string("Invalid ")+name);};
  range("gateDb",-90,0);range("compressorDb",-60,0);range("ratio",1,20);range("essAmount",0,1);range("reverbWet",0,1);range("tuneStrength",0,1);
  if(c.contains("eqGains")){if(!c["eqGains"].is_array()||c["eqGains"].size()!=13)throw std::runtime_error("EQ needs 13 bands");for(auto& x:c["eqGains"])if(!x.is_number()||std::abs(x.get<double>())>18)throw std::runtime_error("Invalid EQ gain");}
  if(c.contains("sampleRate")&&c["sampleRate"]!=44100&&c["sampleRate"]!=48000&&c["sampleRate"]!=96000)throw std::runtime_error("Unsupported sample rate");
  if(c.contains("bufferSize")){int n=c["bufferSize"].get<int>();if(n<128||n>2048||(n&(n-1)))throw std::runtime_error("Invalid buffer size");}
  if(c.contains("key")){std::string k=c["key"];if(k!="C"&&k!="Cm"&&k!="D"&&k!="Dm"&&k!="E"&&k!="Em"&&k!="F"&&k!="Fm")throw std::runtime_error("Invalid key");}
  if(c.contains("backend")&&c["backend"]!="Windows Audio"&&c["backend"]!="ASIO")throw std::runtime_error("Unsupported audio backend");
  for(auto name:{"micChannel","musicLeft","musicRight","monitorLeft","monitorRight","sendLeft","sendRight"})if(c.contains(name)&&(!c[name].is_number_integer()||c[name].get<int>()<0||c[name].get<int>()>255))throw std::runtime_error("Invalid ASIO port index");
  if(c.contains("sendEnabled")&&!c["sendEnabled"].is_boolean())throw std::runtime_error("Invalid second output toggle");
 }
 json enumerate(){json inputs=json::array(),outputs=json::array(),drivers=json::array();for(auto* type:devices.getAvailableDeviceTypes()){type->scanForDevices();if(type->getTypeName()=="Windows Audio"){for(auto s:type->getDeviceNames(true))inputs.push_back(s.toStdString());for(auto s:type->getDeviceNames(false))outputs.push_back(s.toStdString());}else if(type->getTypeName()=="ASIO")for(auto s:type->getDeviceNames(false))drivers.push_back(s.toStdString());}return {{"inputs",inputs},{"outputs",outputs},{"drivers",drivers}};}
 json ports(const std::string& driver){
  if(live||recorder)throw std::runtime_error("Turn LIVE/REC off before reading ASIO ports");
  juce::AudioIODevice* dev=devices.getCurrentAudioDevice();std::unique_ptr<juce::AudioIODevice> probe;
  if(!dev||dev->getTypeName()!="ASIO"||dev->getName()!=juce::String(driver)){
   detach();devices.closeAudioDevice();
   for(auto* type:devices.getAvailableDeviceTypes())if(type->getTypeName()=="ASIO"){type->scanForDevices();if(!type->getDeviceNames(false).contains(juce::String(driver)))throw std::runtime_error("ASIO driver not installed: "+driver);probe.reset(type->createDevice(juce::String(driver),juce::String(driver)));break;}
   dev=probe.get();
  }
  if(!dev)throw std::runtime_error("Cannot initialise ASIO driver: "+driver);
  auto err=dev->getLastError();if(err.isNotEmpty())throw std::runtime_error("ASIO driver: "+err.toStdString());
  json inputs=json::array(),outputs=json::array(),rates=json::array(),buffers=json::array();
  for(auto s:dev->getInputChannelNames())inputs.push_back(s.toStdString());for(auto s:dev->getOutputChannelNames())outputs.push_back(s.toStdString());
  for(auto s:dev->getAvailableSampleRates())rates.push_back(s);for(auto s:dev->getAvailableBufferSizes())buffers.push_back(s);
  return {{"inputs",inputs},{"outputs",outputs},{"sampleRates",rates},{"bufferSizes",buffers},{"driver",driver}};
 }
 void loadSlot(int index,const std::string& path,const std::string& state=""){
  juce::VST3PluginFormat format;juce::OwnedArray<juce::PluginDescription> found;format.findAllTypesForFile(found,juce::String(path));if(found.isEmpty())throw std::runtime_error("No compatible VST3 in "+path);
  juce::String error;auto plugin=formats.createPluginInstance(*found[0],rate,block,error);if(!plugin)throw std::runtime_error(error.toStdString());
  if(!plugin->setBusesLayout({{juce::AudioChannelSet::stereo()},{juce::AudioChannelSet::stereo()}}))throw std::runtime_error("VST3 must support stereo in/out");
  plugin->setRateAndBufferSizeDetails(rate,block);plugin->prepareToPlay(rate,block);if(!state.empty()){juce::MemoryBlock bytes;if(!bytes.fromBase64Encoding(state))throw std::runtime_error("Invalid plugin state");plugin->setStateInformation(bytes.getData(),int(bytes.getSize()));}
  slots.at(index).plugin=std::move(plugin);slots.at(index).path=path;slots.at(index).on=true;slots.at(index).bypass=false;
 }
 json project(){json s=settings;s["version"]=1;s["slots"]=json::array();for(auto& slot:slots){json p={{"path",slot.path},{"on",slot.on},{"bypass",slot.bypass}};if(slot.plugin){juce::MemoryBlock state;slot.plugin->getStateInformation(state);p["state"]=state.toBase64Encoding().toStdString();}s["slots"].push_back(p);}return s;}
public:
 Engine(){formats.addFormat(new juce::VST3PluginFormat());disk.startThread();settings={{"eqGains",std::vector<float>(13,0)}};startTimerHz(25);}
 ~Engine(){stopTimer();detach();music.stop();recorder.reset();for(auto& s:slots)if(s.plugin)s.plugin->releaseResources();devices.closeAudioDevice();disk.stopThread(5000);}
 json command(const json& request){std::string op=request.at("op");auto c=request.value("data",json::object());
  if(op=="devices")return enumerate();
  if(op=="ports")return ports(c.at("driver").get<std::string>());
  if(op=="control-panel"){if(live||recorder)throw std::runtime_error("Turn LIVE/REC off before opening the driver panel");auto* dev=devices.getCurrentAudioDevice();if(!dev||dev->getTypeName()!="ASIO")throw std::runtime_error("Apply ASIO audio first");return {{"opened",dev->showControlPanel()}};}
  if(op=="configure"){if(live||recorder)throw std::runtime_error("Turn LIVE/REC off before changing devices");validate(c);settings.update(c);open(settings);return settings;}
  if(op=="params"){for(auto name:{"backend","driver","input","output","sampleRate","bufferSize","micChannel","musicLeft","musicRight","monitorLeft","monitorRight","sendLeft","sendRight","sendEnabled"})if(c.contains(name))throw std::runtime_error("Use configure to change audio routing");validate(c);detach();try{settings.update(c);applyDSP(settings);if(devices.getCurrentAudioDevice())attach();return settings;}catch(...){if(devices.getCurrentAudioDevice())attach();throw;}}
  if(op=="live"){if(c.value("on",false)){if(!devices.getCurrentAudioDevice())open(settings);if(!routing.asio)music.start(rate);primed=false;live=true;attach();}else{live=false;music.stop();}return {{"live",live.load()},{"source",routing.asio?"ASIO input ports":"WASAPI process loopback"}};}
  if(op=="test"){if(!devices.getCurrentAudioDevice())open(settings);testSamples=int(rate*2);return {{"output",settings.value("output",std::string())},{"duration",2}};}
  if(op=="record"){detach();if(c.value("on",false)){if(!devices.getCurrentAudioDevice()){attach();throw std::runtime_error("Configure audio first");}juce::File file(c.at("path").get<std::string>());if(file.existsAsFile()){attach();throw std::runtime_error("Recording file already exists");}auto stream=file.createOutputStream();if(!stream){attach();throw std::runtime_error("Cannot create WAV");}juce::WavAudioFormat wav;auto writer=std::unique_ptr<juce::AudioFormatWriter>(wav.createWriterFor(stream.get(),rate,2,24,{},0));if(!writer){attach();throw std::runtime_error("Cannot create 24-bit WAV writer");}stream.release();recorder=std::make_unique<juce::AudioFormatWriter::ThreadedWriter>(writer.release(),disk,65536);recordDrops=0;}else recorder.reset();attach();return {{"recording",recorder!=nullptr}};}
  if(op=="slot"){int index=c.at("index");if(index<0||index>=4)throw std::runtime_error("Invalid VST slot");detach();try{auto action=c.at("action").get<std::string>();auto& s=slots.at(index);if(action=="load")loadSlot(index,c.at("path"));else if(action=="remove"){s.plugin.reset();s.path="";}else if(action=="on")s.on=c.at("value");else if(action=="bypass")s.bypass=c.at("value");else if(action=="parameter"){if(!s.plugin)throw std::runtime_error("Empty VST slot");int id=c.at("parameter");auto params=s.plugin->getParameters();float v=c.at("value");if(id<0||id>=params.size()||v<0||v>1)throw std::runtime_error("Invalid plugin parameter");params[id]->setValueNotifyingHost(v);}else throw std::runtime_error("Unknown VST action");json params=json::array();if(s.plugin)for(auto* p:s.plugin->getParameters())params.push_back({{"name",p->getName(60).toStdString()},{"value",p->getValue()}});attach();return {{"path",s.path},{"on",s.on},{"bypass",s.bypass},{"parameters",params}};}catch(...){attach();throw;}}
  if(op=="project-save"){detach();auto p=project();if(devices.getCurrentAudioDevice())attach();return p;}
  if(op=="project-load"){if(live||recorder)throw std::runtime_error("Turn LIVE/REC off before loading project");validate(c);if(c.value("version",0)!=1)throw std::runtime_error("Unsupported project version");settings=c;open(settings);detach();try{for(auto& s:slots)s=Slot{};if(c.contains("slots")){if(c["slots"].size()!=4)throw std::runtime_error("Project needs four VST slots");for(int i=0;i<4;++i){auto p=c["slots"][i];auto path=p.value("path",std::string());if(!path.empty())loadSlot(i,path,p.value("state",std::string()));slots[i].on=p.value("on",true);slots[i].bypass=p.value("bypass",false);}}auto loaded=project();attach();return loaded;}catch(...){attach();throw;}}
  throw std::runtime_error("Unknown command: "+op);
 }
 void audioDeviceAboutToStart(juce::AudioIODevice*)override{}
 void audioDeviceStopped()override{}
 void audioDeviceError(const juce::String& e)override{send({{"event","error"},{"message",e.toStdString()}});}
 void audioDeviceIOCallbackWithContext(const float* const* in,int ins,float* const* out,int outs,int n,const juce::AudioIODeviceCallbackContext&) override {
  juce::ScopedNoDenormals noDenormals;for(int ch=0;ch<outs;++ch)if(out[ch])juce::FloatVectorOperations::clear(out[ch],n);if(n>vocal.getNumSamples()){++underruns;return;}vocal.clear();float pm=0,pu=0,pt=0;bool active=live.load();
  for(int i=0;i<n;++i){float mic=active?Routing::read(in,ins,routing.asio?routing.mic:0,i):0;pm=std::max(pm,std::abs(mic));float v=active?dsp.process(mic):0;vocal.setSample(0,i,v);vocal.setSample(1,i,v);}
  if(active){juce::AudioBuffer<float> b(vocal.getArrayOfWritePointers(),2,n);midi.clear();for(auto& s:slots)if(s.plugin&&s.on&&!s.bypass)s.plugin->processBlock(b,midi);}
  if(active&&!routing.asio&&!primed&&music.queue.size()>=unsigned(block*2))primed=true;
  // Keep latency bounded when independent capture/output clocks drift.
  if(music.queue.size()>unsigned(rate*.15))for(int drop=0;drop<n&&music.queue.size()>unsigned(block*3);++drop)music.queue.pop();
  for(int i=0;i<n;++i){auto m=active?(routing.asio?routing.music(in,ins,i):primed?music.queue.pop():std::array<float,2>{}):std::array<float,2>{};pu=std::max({pu,std::abs(m[0]),std::abs(m[1])});int remaining=testSamples.load();float test=0;if(remaining>0){testSamples.fetch_sub(1);test=.10f*std::sin(float(oscillator));oscillator+=2*juce::MathConstants<double>::pi*440/rate;if(oscillator>2*juce::MathConstants<double>::pi)oscillator-=2*juce::MathConstants<double>::pi;}
   float rawL=(vocal.getSample(0,i)*micVolume+m[0]*musicVolume)*masterVolume+test,rawR=(vocal.getSample(1,i)*micVolume+m[1]*musicVolume)*masterVolume+test;float maximum=std::max(std::abs(rawL),std::abs(rawR));float desired=maximum>.97f?.97f/maximum:1.f;limiterGain=desired<limiterGain?desired:limiterGain+.0005f*(desired-limiterGain);float l=0,r=0;for(int ch=0;ch<2;++ch){float value=(ch==0?rawL:rawR)*limiterGain;value=std::clamp(std::isfinite(value)?value:0.f,-.97f,.97f);vocal.setSample(ch,i,value);if(ch==0)l=value;else r=value;pt=std::max(pt,std::abs(value));}routing.write(out,outs,i,l,r);if(++waveCount>=unsigned(std::max(1,int(rate/3200)))){waveform[waveIndex++%128]=(l+r)*.5f;waveCount=0;}
  }
  peaks[0]=pm;peaks[1]=pu;peaks[2]=pt;if(recorder&&!recorder->write(vocal.getArrayOfReadPointers(),n))++recordDrops;
 }
 void timerCallback()override{auto loopError=music.takeError();if(!loopError.empty())send({{"event","error"},{"message",loopError}});json w=json::array();for(auto& x:waveform)w.push_back(x.load());send({{"event","meters"},{"mic",peaks[0].load()},{"music",peaks[1].load()},{"master",peaks[2].load()},{"waveform",w},{"live",live.load()},{"recording",recorder!=nullptr},{"loopback",music.healthy.load()},{"loopbackError",music.error.load()},{"captureDrops",music.queue.dropped.load()},{"recordDrops",recordDrops.load()},{"underruns",underruns.load()},{"detectedKey",dsp.detectedKey.load()}});}
};
int main(int argc,char** argv){
 if(argc>1&&std::string(argv[1])=="--self-test"){VocalDSP dsp;dsp.prepare(48000);for(auto& b:dsp.eq)b.peak(48000,1000,0);double sum=0;for(int i=0;i<96000;++i){float x=dsp.process(.2f*std::sin(float(2*3.141592653589793*440*i/48000)));if(!std::isfinite(x))return 2;sum+=x*x;}if(sum<1)return 3;std::cout<<"DSP self-test passed\n";return 0;}
 juce::ScopedJuceInitialiser_GUI juceRuntime;auto engine=std::make_unique<Engine>();
 std::thread input([&]{std::string line;while(std::getline(std::cin,line)){try{auto request=json::parse(line);if(request.value("op",std::string())=="quit")break;juce::MessageManager::callAsync([&,request]{auto id=request.value("id",0);try{send({{"id",id},{"ok",true},{"data",engine->command(request)}});}catch(const std::exception& e){send({{"id",id},{"ok",false},{"error",e.what()}});}});}catch(const std::exception& e){send({{"event","error"},{"message",e.what()}});}}juce::MessageManager::callAsync([]{juce::MessageManager::getInstance()->stopDispatchLoop();});});
 send({{"event","ready"},{"backend","JUCE ASIO + WASAPI native engine"}});juce::MessageManager::getInstance()->runDispatchLoop();input.join();return 0;
}
