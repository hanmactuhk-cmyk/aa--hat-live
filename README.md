# HNStudio Musik AI 1.1.0

Electron UI + native Windows x64 JUCE engine. Native audio is built in Release by GitHub Actions, packaged into both installer and portable EXE. UI animation runs outside the native audio process.

## ASIO Link Pro mode

This app hosts an installed 64-bit ASIO driver. It does not embed or install ASIO Link Pro itself, change the Windows default playback device, or program its proprietary routing profile.

1. Install ASIO Link Pro x64 and select its hardware ASIO driver in its routing panel.
2. Route Windows playback to its ASIOVADPRO playback endpoint. Route that endpoint into separate host MUSIC input ports (e.g. LinkIn3/LinkIn4). Disable any parallel direct music-to-speaker route in the driver.
3. In HNStudio select ASIO, the ASIO Link Pro driver, and Read ASIO ports. Select MIC input (e.g. Input1), MUSIC Left/Right, Monitor Left/Right (e.g. Output1/Output2). Names come from the installed driver; all ports are selectable.
4. Enable second Master output pair when needed (e.g. LinkOut3/LinkOut4). Route that pair through the ASIO Link Pro virtual recording endpoint selected in TikTok Studio. Keep its return out of the MUSIC input to prevent feedback. Do not also select the unprocessed mic/system audio in TikTok.
5. Apply audio, TEST 440 Hz, then LIVE ON. The tone actually goes through the selected native output ports. Save Project preserves routing and effects.

The initial port suggestions use names from the reference Cubase setup where available. They cannot infer an arbitrary external ASIO Link Pro routing profile. Driver control panel is available while LIVE/REC are off; Apply again after changes. Driver installation and initial routing are still required before opening-and-using the app. No separate VB-CABLE is needed for this ASIO route.

## Signal flow

MIC ASIO input → Gate → Compressor → EQ13 → De-Esser → Pitch correction → Reverb → VST3 slots → Mic Volume.

Windows audio → ASIO Link Pro virtual playback → selected MUSIC input pair → Music Volume.

MIC + MUSIC → Master Volume → Limiter → Monitor output pair + optional second Master output pair. Recorder writes the same stereo Master as 24-bit WAV using a background writer.

ASIO mode does not start WASAPI loopback. WASAPI remains selectable for ordinary Windows devices; loopback copies system playback and does not silence its direct path, so using the same hardware output can duplicate music.

## Build

Windows x64, Node22, CMake3.24+, VS2022 C++ and Windows SDK:

```
npm ci
npm run check
npm run build:native
build\native\Release\hn-dsp-selftest.exe
build\native\Release\hn-routing-selftest.exe
node tests/backend-smoke.cjs
npm run dist
```

GitHub Actions performs these steps and uploads HNStudio-Musik-AI-Windows-x64 with installer and portable EXE. CI verifies build/package integrity, backend startup, DSP behavior and channel routing. It cannot verify physical microphones, speakers, installed third-party ASIO drivers or TikTok capture. Pitch correction/Auto Key are custom DSP, not Antares Auto-Tune. VST3 uses generic parameter controls.

## License

Project: AGPL-3.0-only. JUCE is used under its open-source licensing terms. ASIO interface headers from Steinberg are used under their GPLv3 alternative; see native/vendor/asio/LICENSE.txt and README.md. ASIO Link Pro is an external driver and is not redistributed.
