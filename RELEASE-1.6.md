# HNStudio Musik AI 1.6

Live vocal inserts are now small capsules. All parameter panels are non-modal: drag their header, continue using the mixer underneath, minimize with − and restore from the bottom dock, or close with ×.

Reverb now uses a four-line feedback network with controllable decay (0.25–5 s), damping and pre-delay. The dry vocal stays present. Three live presets set real native parameters. Short/long delay feedback is softly bounded. Low-cut, warmth saturation and air enhancement are optional microphone-only native effects; music continues to bypass vocal inserts.

AutoTune has an optional local AI confidence guard: a trained 4→8→1 neural classifier rejects uncertain periodicity before the existing YIN detector supplies notes to the DSP pitch shifter. It does **not** generate or reconstruct singing, and the pitch correction itself remains DSP. The bundled small model is trained on synthetic harmonic/noise audio using `tools/train_pitch_guard.py`; its held-out synthetic accuracy is not evidence of real singer accuracy. Real vocal recordings and XOX K10 hardware still require listening tests. No cloud service, API key or Python dependency is needed to run the app.

Verification: native tests cover actual A440 output correction, automatic key detection, microphone/music isolation, echo impulses, decaying reverb tail, low-cut response, and finite warmth/air output. Electron interaction tests check non-modal operation, minimize/restore, all faders issuing commands and existing multitrack interactions. Windows CI builds and packages Release x64.
