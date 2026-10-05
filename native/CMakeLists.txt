cmake_minimum_required(VERSION 3.24)
project(HNAudio VERSION 1.7.0 LANGUAGES C CXX)
if(NOT WIN32)
 message(FATAL_ERROR "Native audio requires Windows x64 and Windows SDK")
endif()
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
include(FetchContent)
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.6 GIT_SHALLOW TRUE)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(JUCE json)
add_executable(hn-audio engine.cpp)
target_compile_definitions(hn-audio PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00 NTDDI_VERSION=0x0A00000A JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1 JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_PLUGINHOST_VST3=1 JUCE_PLUGINHOST_VST=0 JUCE_ASIO=1 JUCE_WASAPI=1 NOMINMAX WIN32_LEAN_AND_MEAN)
target_include_directories(hn-audio PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/vendor/asio)
target_link_libraries(hn-audio PRIVATE juce::juce_audio_utils juce::juce_dsp nlohmann_json::nlohmann_json ole32 mmdevapi)
if(MSVC)
 target_compile_options(hn-audio PRIVATE /W4 /EHsc /utf-8)
endif()

add_executable(hn-routing-selftest ../tests/routing_test.cpp)
target_compile_features(hn-routing-selftest PRIVATE cxx_std_20)
target_include_directories(hn-routing-selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# Standalone DSP test: no Electron, GUI, audio devices or backend startup.
add_executable(hn-dsp-selftest ../tests/dsp_test.cpp)
target_compile_features(hn-dsp-selftest PRIVATE cxx_std_20)
target_include_directories(hn-dsp-selftest PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
if(MSVC)
 target_compile_options(hn-dsp-selftest PRIVATE /W4 /EHsc /utf-8)
endif()

add_executable(hn-audio-regression ../tests/audio_regression.cpp)
target_compile_features(hn-audio-regression PRIVATE cxx_std_20)

# Bundled, original stereo Delay VST3. The live host places this AFTER mic inserts.
juce_add_plugin(HNDelay
 COMPANY_NAME "Hoai Nguyen Studio"
 BUNDLE_ID "studio.hoai.hndelay"
 PLUGIN_MANUFACTURER_CODE Hnst
 PLUGIN_CODE Hndl
 FORMATS VST3
 PRODUCT_NAME "HNStudio Delay"
 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
 COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(HNDelay PRIVATE plugins/delay_plugin.cpp)
target_link_libraries(HNDelay PRIVATE juce::juce_audio_utils)
target_compile_definitions(HNDelay PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
if(MSVC)
 target_compile_options(HNDelay PRIVATE /utf-8)
endif()
juce_add_plugin(HNMaster
 COMPANY_NAME "Hoai Nguyen Studio"
 BUNDLE_ID "studio.hoai.hnmaster"
 PLUGIN_MANUFACTURER_CODE Hnst PLUGIN_CODE Hnms
 FORMATS VST3 PRODUCT_NAME "HNStudio Master"
 IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
 COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(HNMaster PRIVATE plugins/master_plugin.cpp)
target_link_libraries(HNMaster PRIVATE juce::juce_audio_utils)
target_compile_definitions(HNMaster PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0)
if(MSVC)
 target_compile_options(HNMaster PRIVATE /utf-8)
endif()
add_executable(hn-postfx-selftest ../tests/postfx_test.cpp)
target_compile_features(hn-postfx-selftest PRIVATE cxx_std_20)
