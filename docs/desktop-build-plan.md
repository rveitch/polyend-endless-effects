# Desktop build implementation, 2026-09-30

Ryan approved proceeding with the described VST3 build and REAPER installation/verification. Execute inline in the existing checkout; preserve pedal DSP, SDK, binaries and user projects. No push or pedal deployment.

## Design

A separate JUCE 8.0.4 CMake target reads the effect catalog and dependency lock. Build native arm64 VST3 with distinct effect identities. Each processor owns a typed factory-created effect and 2400000-float working buffer. The official Patch ABI has no virtual destructor, so use a concrete deleter. Device builds retain the existing singleton path; only conditional desktop factory glue is added.

Juno controls: Mix, Tone, Width, Base Mode I/II, I+II and desktop Bypass. Mode parameters drive existing press/hold actions on the audio thread and persist through host state. Passthrough provides a second selectable target. Future effects need explicit desktop metadata and action mapping; do not assume Juno's action semantics apply to other algorithms.

Enforce stereo 48000 Hz; unsupported rates pass dry with a visible warning. Process in bounded chunks using preallocated scratch storage, advancing wet state during bypass. XML parameter state validates its schema before replacement. No resampler, new DSP, plugin distribution or firmware changes.

## Steps and acceptance

1. RED: finite processor tests for independent instances, reference parity, rate/bypass behavior, reset, state restoration and oversized callbacks; CLI rejects unknown effects.
2. Implement conditional factories, JUCE processor/editor, catalog-selected CMake target and finite build CLI. Pin/check JUCE sources before configuring. Use JUCE under its own license; record source provenance.
3. GREEN: native processor tests, actual VST3 build, existing 23-tool suite, all host/ARM effects and baseline DSP comparison. Check applicable IDE inspections.
4. One independent read-only review; fix material issues with regressions. Verify bundle architecture/signature, install the tested new bundle under user VST3 folder without overwriting an unrelated bundle. Check REAPER discovery/loading in a separate test project, preserving existing projects and settings.
5. Commit documentation and code locally. Report actual plugin path, verification limits and the concrete REAPER listening setup.

Implementation details use JUCE's official module APIs. The reviewed fork is an architectural reference, not copied DSP or a dependency on its extended header. User approval covers continuous execution of this presented build/install workflow without extra approval pauses.

## Completion evidence

Implemented and independently reviewed inline. Native processor suite passed 1/1, tooling passed 24/24, both ARM builds and host regressions passed, and the 12-second migration comparison was bit-identical across modes and transitions. Both catalog desktop bundles build. Juno's arm64 signature verified before and after user-folder installation. REAPER 7.80 scanned and loaded the plugin in a separate saved project tab without restarting or playing audio. IDE inspection returned File not found for the new source. State loading updates controls; prepare/recreation resets history. Raw logs, manifests, install receipt and REAPER project are preserved under ignored build directories. Hardware CPU/authenticity and musical listening remain separate validation.
