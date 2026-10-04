# HNStudio Musik AI 1.3.0

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

## v1.2 studio layout and mic-only processing

Audio Settings, built-in FX, EQ, AutoTune, project operations and VST parameters open in dialogs from the compact studio dashboard. The signal graph has separate vocal, dry music and master buffers. The vocal DSP and VST3 inserts only see the selected MIC port. Music gets Music Volume, then the shared Master Volume and protective limiter. Use MIC ONLY / MUSIC ONLY to diagnose input routing. If music is already present in the MIC input supplied by XOX/ASIO Link Pro, the app cannot split that premixed input into independent sources; remove the music route into that MIC port in the driver/hardware setup.

Auto Vocal measures six seconds of the live microphone and derives gate threshold, compressor threshold/ratio, EQ, de-essing, wet level and mic gain. It rejects insufficient/silent input. The overall Auto amount adjusts the measured vocal treatment; individual faders can override the result. External VST parameters stay manual. AutoTune offers ON/OFF, scale or chromatic correction, correction strength and retune speed, with detected/target pitch telemetry. Auto Key estimates a major/minor key from microphone note history; it cannot reliably infer accompaniment key from a single vocal note.

Regression tests verify dry music stays sample-identical through an aggressively configured mic DSP/mock VST chain, actual 450 Hz mic output is corrected toward A440, and Auto Vocal settings react to input level at 44.1/48/96 kHz. Windows CI also launches the actual Electron renderer with a clearly mocked audio bridge to check popup/control wiring and capture UI previews. These UI tests do not establish physical ASIO routing correctness.

## v1.3 Auto Key, echo, localization and crisp neon

Auto Key is enabled by default. It waits for several distinct microphone notes before locking a major/minor key, then that key is applied directly in the pitch corrector and reflected in the key display. While evidence is insufficient, correction uses the nearest chromatic note. The manual key selection is disabled while Auto Key is on. Detect key again resets the note history. Existing saved projects retain their explicit Auto Key setting. This is microphone-based key estimation, not a guarantee of the accompaniment key.

Short Echo and Long Echo are independent mic-only inserts with ON/OFF, level, delay time and feedback controls. EQ now publishes 13 real microphone band levels to illuminate the vertical faders. These visuals run in the Electron renderer; DSP stays in the native process.

Settings includes Vietnamese / English with an Apply button, persisted in the local Electron profile. The audio settings and effect dialogs use sharper solid surfaces, compact padding, round corners and stronger moving neon edges without backdrop blur. About includes Hoài Nguyễn Studio and Zalo 0965.043.000. The top-center perforated ticker scrolls the requested introductory text right to left, translated in English mode.

Audio regressions also verify multi-note automatic key detection, rejection of a single-note key guess, short/long echo timing and EQ activity driven by an actual 1kHz mic input. Electron UI tests cover language switching, automatic key display, popup controls and the About contact information.

## Recording workspace (v1.4)
Use RECORDING to switch to the native four-track desk: Beat + Mic 1/2/3. Import audio, select a Mic track and Record; Stop commits the dry mono microphone take at its timeline position. Takes are stored in Documents/HNStudio Takes. Track playback supports independent gain/pan, mute/solo, start offset, non-destructive trim, built-in vocal FX and four VST3 inserts per track. Audio assets are preloaded and decoded off the audio callback, with bounded memory. WAV 24-bit writers run on the disk thread. The timeline replaces the live system MUSIC input during transport, keeping microphone monitoring available. Export renders track FX and VST3 to stereo 24-bit WAV with a two-second tail and a protective peak clamp. This initial editor has one clip per track; new takes replace the visible clip while retaining older take files.

Save recording project copies audio assets into a companion .hnrec.media folder. Keep that folder with the project; external VST3 binaries are not copied and must remain installed. Plugins must be Windows x64 VST3 with stereo in/out. The Installed VST3 scan looks in the common system/user VST3 directories; use Load file for custom locations. An EXE such as an Auto-Tune installer must first install the actual plugin on Windows; it is not an audio processor that can be loaded directly.

Limitations: maximum four tracks, one clip per track, no automation lanes or plugin latency compensation. Use zero-latency/realtime insert settings for overdubs; this is a first multitrack workspace, not a full DAW. Recording limit is ten minutes or 32 million samples per take; imported audio max thirty minutes / 32 million samples per track, total 64 million stereo frames. Device-reported input/output latency is subtracted from new take placement; actual timing depends on the driver and plugin settings. No physical-device or commercial-plugin validation is performed in CI.
