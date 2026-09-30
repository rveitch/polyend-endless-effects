# Desktop audition evaluation, 2026-09-30

The fork's desktop wrapper is useful reference material for a future audition tool. It was evaluated, not imported, built or installed. Current finite stereo captures already provide reproducible offline audition files.

Reviewed fork commit: `29761ec0ca83a1f68a6b1d04376ff99db36b0f77`.

## Source findings

The [CMake configuration](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/vst/CMakeLists.txt) requires CMake 3.22, fetches JUCE 8.0.4 and offers VST3, LV2 and Standalone. Its effect lookup expects top-level `effects/<name>.cpp`; ours needs catalog-aware selection and vendored-header paths. It also applies `-fsingle-precision-constant` to GNU/Clang effect compilation, which must be checked on Apple Clang rather than assumed portable.

The [processor](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/vst/src/PluginProcessor.cpp) processes distinct stereo buffers, supplies working memory and delivers actions/changed parameters during processing. Each instance obtains the same SDK singleton, so simultaneous instances share state and can replace its working buffer. A future wrapper must enforce one instance or introduce a factory outside the device ABI. Its block-local non-48-kHz path pads discarded tails with zeros; I would enforce 48 kHz until a persistent, tested streaming converter exists.

The [wrapper README](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/vst/README.md) reports Linux-only verification. Its footswitch descriptions do not define our Juno controls. Host output is not evidence of Cortex-M7 parity.

JUCE 8.0.4 has its own [AGPL/commercial licensing terms](https://github.com/juce-framework/JUCE/blob/8.0.4/LICENSE.md), separate from the SDK license. Evaluate the intended use against those terms before adopting it.

## Local prerequisites and next implementation boundary

Apple's command-line tools and C++20 compiler are present (`/Library/Developer/CommandLineTools`). CMake was absent from PATH at evaluation time. No packages or plugins were installed. A macOS build and REAPER loading remain unverified.

A later implementation should select one catalog effect, enforce stereo 48 kHz, allocate working memory before processing, queue UI actions onto the audio thread, and test parameter resets and transitions. Include an explicit desktop bypass separate from patch actions, unique plugin identities per effect and robust multiple-instance handling. Compare desktop renders against finite captures before device listening. Desktop and device artifacts should have distinct output/provenance records.
