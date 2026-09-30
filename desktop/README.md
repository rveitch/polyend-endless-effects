# Desktop VST3 audition

Build and test the Juno plugin with the pinned local JUCE checkout:

```sh
python3 scripts/desktop.py test --effect junoChorus
```

The build produces `build/desktop/junoChorus/EndlessPlugin_artefacts/Release/VST3/Endless_JunoChorus.vst3`. It uses native architecture on macOS, separate from device output. `desktop.manifest.json` identifies the dependency revisions and bundle hashes. The compiler never uses the ARM-only float-literal flag on Apple Clang.

Set REAPER to stereo 48000 Hz, disable per-plugin oversampling, and use the native Apple Silicon application with an arm64 build. Other rates pass dry and show a warning. The supported desktop controls are Mix, Tone, Width, Base mode I/II, I+II and Bypass. These preserve the Juno DSP mappings, including the diagnostic Mix law. Mode controls invoke patch actions during audio processing. Bypass returns original samples while wet state continues advancing. Saved plugin state includes knobs, modes and bypass; loading state changes controls; preparing or recreating the processor resets DSP history.

Every processor owns a separately created effect and working buffer. Project-owned desktop factory glue is conditional; device builds retain their original singleton entry point and the vendored SDK remains unchanged. The host uses a typed deleter because the official Patch interface lacks a virtual destructor.

Passthrough is selectable with `python3 scripts/desktop.py build --effect passthrough`; its controls/actions have no audio effect. Register desktop identity and parameter names in the effect catalog when adding another effect, implement its optional factory and adapt the wrapper's action mapping when it differs from the Juno I/II/combined model. Do not assume other effects share that state machine.

The build verifies JUCE's full locked commit and clean checkout before configuring. JUCE is governed by its [own license](https://github.com/juce-framework/JUCE/blob/51d11a2be6d5c97ccf12b4e5e827006e19f0555a/LICENSE.md). The fork was an architectural reference; wrapper code here uses JUCE APIs with project-specific lifetime, parameter/state and bypass handling.

No desktop render establishes pedal CPU performance or circuit authenticity. Use the existing raw stereo capture workflow and hardware validation record for final voicing decisions.

## Local installation and REAPER check, 2026-09-30

The Juno bundle is installed at `/Users/ryanveitch/Library/Audio/Plug-Ins/VST3/Endless_JunoChorus.vst3`. REAPER 7.80 discovered it using Preferences > Plug-ins > VST > Re-scan > Re-scan VST paths for new/modified plug-ins, without a restart. It loaded as **VST3: Endless_JunoChorus (Ryan Veitch)** with all six controls and the stereo 48 kHz status visible. A separate blank test tab was saved under `build/desktop/reaper-check/Endless Juno Desktop Check.RPP`; the existing modified comparison project was preserved. No playback or musical listening evaluation was performed.

For audition, add this FX to a stereo music track in a 48000 Hz project with plugin oversampling disabled. Start Mix at noon, Tone at noon, Width at noon, Base mode I and I+II off. Compare I, II and I+II at matched levels. Maximum Mix gives fully wet output for measurements. Other project rates pass dry with a warning.

Native processor tests cover direct effect parity in all three modes, independent instances, state round trips and malformed-state rejection, chunked oversized callbacks, exact bypass and unsupported rates. Both ARM effects and host regressions pass; 12-second baseline captures remain bit-identical in I, II, I+II and transitions. Passthrough also builds as its own VST3. The IDE connector could not resolve the new processor file, so compiler warnings-as-errors and test results provide code validation. Installation hashes are retained in ignored `build/desktop/install-receipt.json`, and build logs/manifests sit under `build/desktop/`.
