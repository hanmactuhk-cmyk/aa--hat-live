#pragma once
#include <windows.h>
#include <audioclient.h>
#include <audioclientactivationparams.h>
#include <mmdeviceapi.h>
#include <wrl.h>
#include <wrl/implements.h>
#include <atomic>
#include <thread>
#include <future>
#include <mutex>
#include <string>
#include <array>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
inline void hrCheck(HRESULT hr,const char* operation) { if(FAILED(hr)) throw std::runtime_error(std::string(operation)+" HRESULT="+std::to_string(static_cast<unsigned long>(hr))); }
// SPSC stereo queue: only capture thread writes, audio callback reads.
struct StereoQueue {
 static constexpr unsigned capacity=131072;
 std::array<std::array<float,2>,capacity> data{};
 std::atomic<unsigned> write{0},read{0}; std::atomic<unsigned> dropped{0};
 void push(float l,float r) { auto w=write.load(std::memory_order_relaxed); auto n=(w+1)%capacity; if(n==read.load(std::memory_order_acquire)){ ++dropped; return; } data[w]={l,r};write.store(n,std::memory_order_release); }
 std::array<float,2> pop(){auto r=read.load(std::memory_order_relaxed);if(r==write.load(std::memory_order_acquire))return {};auto v=data[r];read.store((r+1)%capacity,std::memory_order_release);return v;}
 unsigned size()const{return (write.load()-read.load()+capacity)%capacity;}
 void reset(){read=0;write=0;dropped=0;}
};
class Activation final : public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,Microsoft::WRL::FtmBase,IActivateAudioInterfaceCompletionHandler> {
public:
 HANDLE event=CreateEventW(nullptr,FALSE,FALSE,nullptr); HRESULT result=E_PENDING; ComPtr<IAudioClient> client;
 ~Activation(){CloseHandle(event);}
 STDMETHOD(ActivateCompleted)(IActivateAudioInterfaceAsyncOperation* operation) override { ComPtr<IUnknown> unknown; HRESULT activation=E_FAIL; result=operation->GetActivateResult(&activation,&unknown);if(SUCCEEDED(result))result=activation;if(SUCCEEDED(result))result=unknown.As(&client);SetEvent(event);return S_OK; }
};
class Loopback {
 std::thread worker;std::atomic<bool> running{false};std::mutex errorMutex;std::string lastError;
public:
 std::string takeError(){std::lock_guard lock(errorMutex);auto value=lastError;lastError.clear();return value;}
 StereoQueue queue;std::atomic<bool> healthy{false};std::atomic<unsigned long> error{0};
 void start(double rate) {
  stop();queue.reset();error=0;
  std::promise<std::string> startup;auto future=startup.get_future();running=true;
  worker=std::thread([this,rate,p=std::move(startup)]() mutable {
   HRESULT apartment=CoInitializeEx(nullptr,COINIT_MULTITHREADED);
   bool announced=false;
   try {
    hrCheck(apartment,"Loopback COM");
    AUDIOCLIENT_ACTIVATION_PARAMS params{};params.ActivationType=AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK;params.ProcessLoopbackParams.TargetProcessId=GetCurrentProcessId();params.ProcessLoopbackParams.ProcessLoopbackMode=PROCESS_LOOPBACK_MODE_EXCLUDE_TARGET_PROCESS_TREE;
    PROPVARIANT pv{};pv.vt=VT_BLOB;pv.blob.cbSize=sizeof(params);pv.blob.pBlobData=reinterpret_cast<BYTE*>(&params);
    auto handler=Microsoft::WRL::Make<Activation>();ComPtr<IActivateAudioInterfaceAsyncOperation> operation;
    hrCheck(ActivateAudioInterfaceAsync(VIRTUAL_AUDIO_DEVICE_PROCESS_LOOPBACK,__uuidof(IAudioClient),&pv,handler.Get(),&operation),"Process loopback activation (requires Windows build >=20348)");
    if(WaitForSingleObject(handler->event,10000)!=WAIT_OBJECT_0)throw std::runtime_error("Loopback activation timed out");
    hrCheck(handler->result,"Loopback activation completion");auto client=handler->client;
    WAVEFORMATEX fmt{};fmt.wFormatTag=WAVE_FORMAT_IEEE_FLOAT;fmt.nChannels=2;fmt.nSamplesPerSec=static_cast<DWORD>(rate);fmt.wBitsPerSample=32;fmt.nBlockAlign=8;fmt.nAvgBytesPerSec=fmt.nSamplesPerSec*8;
    hrCheck(client->Initialize(AUDCLNT_SHAREMODE_SHARED,AUDCLNT_STREAMFLAGS_LOOPBACK|AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM|AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY,200000,0,&fmt,nullptr),"Loopback initialize");
    ComPtr<IAudioCaptureClient> capture;hrCheck(client->GetService(IID_PPV_ARGS(&capture)),"Loopback capture service");hrCheck(client->Start(),"Loopback start");healthy=true;p.set_value("");announced=true;
    while(running){UINT32 packet=0;hrCheck(capture->GetNextPacketSize(&packet),"Loopback packet");while(packet){BYTE* data=nullptr;UINT32 frames=0;DWORD flags=0;hrCheck(capture->GetBuffer(&data,&frames,&flags,nullptr,nullptr),"Loopback read");auto* samples=reinterpret_cast<float*>(data);for(UINT32 i=0;i<frames;++i){bool silent=(flags&AUDCLNT_BUFFERFLAGS_SILENT)!=0;queue.push(silent?0:samples[i*2],silent?0:samples[i*2+1]);}hrCheck(capture->ReleaseBuffer(frames),"Loopback release");hrCheck(capture->GetNextPacketSize(&packet),"Loopback next packet");}Sleep(2);}
    client->Stop();
   }catch(const std::exception& e){healthy=false;error=1;{std::lock_guard lock(errorMutex);lastError=e.what();}if(!announced)p.set_value(e.what());}
   healthy=false;if(SUCCEEDED(apartment))CoUninitialize();
  });
  auto message=future.get();if(!message.empty()){stop();throw std::runtime_error(message);}
 }
 void stop(){running=false;if(worker.joinable())worker.join();healthy=false;}
 ~Loopback(){stop();}
};
