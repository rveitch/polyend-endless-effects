# Desktop audition evaluation, 2026-09-30

The fork's desktop wrapper is useful reference material for a future audition tool. It was evaluated, not imported, built or installed. Current finite stereo captures already provide reproducible offline audition files.

Reviewed fork commit: `29761ec0ca83a1f68a6b1d04376ff99db36b0f77`.

## Source findings

The [CMake configuration](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/vst/CMakeLists.txt) requires CMake 3.22, fetches JUCE 8.0.4 and offers VST3, LV2 and Standalone. Its effect lookup expects top-level `effects/<name>.cpp`; ours needs catalog-aware selection and vendored-header paths. It also applies `-fsingle-precision-constant` to GNU/Clang effect compilation, which must be checked on Apple Clang rather than assumed portable.

The [processor](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/vst/src/PluginProcessor.cpp) processes distinct stereo buffers, supplies working memory and delivers actions/changed parameters during processing. Each instance obtains the same SDK singleton, so simultaneous instances share state and can replace its working buffer. A future wrapper must enforce one instance or introduce a factory outside the device ABI. Its block-local non-48-kHz path pads discarded tails with zeros; I would enforce 48 kHz until a persistent, tested streaming converter exists.

The [wrapper README](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/vst/README.md) reports Linux-only verification. Its footswitch descriptions do not define our Juno controls. Host output is not evidence of Cortex-M7 parity.

JUCE 8.0.4 has its own [AGPL/commercial licensing terms](https://github.com/juce-framework/JUCE/blob/8.0.4/LICENSE.md), separate from the SDK license. Evaluate the intended use against those terms before adopting it.

## Local prerequisites and next implementation boundary

Apple's command-line tools and C++20 compiler are present (`/Library/Developer/CommandLineTools`). CMake was absent from PATH at evaluation time. The dependency setup below was subsequently requested and completed. The effect wrapper build and REAPER loading remain unverified.

A later implementation should select one catalog effect, enforce stereo 48 kHz, allocate working memory before processing, queue UI actions onto the audio thread, and test parameter resets and transitions. Include an explicit desktop bypass separate from patch actions, unique plugin identities per effect and robust multiple-instance handling. Compare desktop renders against finite captures before device listening. Desktop and device artifacts should have distinct output/provenance records.

## Dependency setup completed, 2026-09-30

Ryan authorized installing CMake and fetching pinned JUCE sources. Homebrew installed CMake 4.4.3 at `/opt/homebrew/bin/cmake`. Automatic Homebrew updates and install cleanup were disabled for this invocation to keep the installation focused.

`desktop.lock.json` records JUCE 8.0.4 at full commit `51d11a2be6d5c97ccf12b4e5e827006e19f0555a`. Its local source checkout is ignored `build/desktop/dependencies/JUCE/`; the downloaded framework is not committed or installed globally. The initial fetch used the release tag, and its HEAD was checked against the official tag commit. Future desktop CMake configuration should use the full commit from the lock, or validate this local checkout before using it.

To reproduce the dependency fetch from a fresh clone:

```sh
brew install cmake
mkdir -p build/desktop/dependencies
git clone --depth 1 --branch 8.0.4 https://github.com/juce-framework/JUCE.git build/desktop/dependencies/JUCE
git -C build/desktop/dependencies/JUCE rev-parse HEAD
```

The final command must report the full commit above. Avoid cloning into an existing dependency folder. No REAPER plugin has been built or installed. The next step is adapting the desktop wrapper to the effect catalog and official SDK paths, including state isolation and stereo 48 kHz processing.

The native Apple Silicon prerequisite check also passed:

```sh
cmake -S build/desktop/dependencies/JUCE -B build/desktop/juce-check \
  -DJUCE_BUILD_EXTRAS=OFF -DJUCE_BUILD_EXAMPLES=OFF \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_ARCHITECTURES=arm64
```

CMake configured and generated successfully using Apple Clang 15.0.0. It built and tested `juceaide`, JUCE's build-time helper. The JUCE source checkout remained clean. This checks framework/toolchain setup; it does not test our effect wrapper or plugin loading.
