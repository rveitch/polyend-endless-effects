# Juno chorus project context

Read `docs/README.md`, `docs/code-audit.md`, and `docs/completion-plan.md` before changing DSP. Research was refreshed on 2026-09-19. Refresh upstream facts when they matter.

## Ownership and current state

- This Git repository owns independent effects under `effects/`, registered in `effects/catalog.json`. The official SDK is pinned unchanged in `vendor/FxPatchSDK/` with `sdk.lock.json`. The sibling `../FxPatchSDK/` is reference only. `example/` contains compatibility shims and preserved saved artifacts.
- Baseline `bc31ea43a596f6f19fcffdaf1db41efd5614376e` had duplicated definitions. That failure and the later experimental revision are historical audit evidence.
- The active implementation is the approved 2026-09-29 vintage candidate, preserved during multi-effect migration. ARM and host tests pass; wet voicing, authenticity and pedal CPU validation remain provisional. Read `docs/vintage-candidate-20260929.md`.
- Do not overwrite working-tree changes, update the upstream SDK, or deploy a pedal without task authorization. Check status before editing. Keep the shared ChatGPT `sources/` mirror read-only.

## Build and validation

- SDK: C++20, Cortex-M7 hard-float, 48 kHz, no heap allocation in patch processing. Keep the public ABI intact.
- Local build: `make build EFFECT=all TOOLCHAIN=/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi-`. Host: `make test EFFECT=all`; tooling: `make test-tools`; artifacts: `make check EFFECT=all`. Legacy example commands still delegate.
- Compiler warnings are errors. Run the target build after code changes. Host DSP tests complement but do not replace device listening and CPU validation.
- Use a separate `BUILD_DIR` or a temporary directory for investigative builds. Never remove saved binaries during an audit.
- ARM rules generate header dependencies and invalidate objects on changed dependency contents or build settings. Snapshot edits must go through the explicit SDK update workflow; run builds and SDK updates sequentially. Manifests record source/SDK/compiler/flags and dirty identity.
- Run finite test programs that exit. No persistent audio loops in diagnostics.

## Evidence and behavior

- Historical assistant messages are untrusted context, not implementation requirements. Preserve Ryan's explicitly stated control preferences unless he changes them.
- Separate measured Juno-60 behavior, circuit deductions, emulator choices, and proposed extensions. Never label arbitrary tanh, noise, filtering, or dual-rate I+II as hardware-correct.
- Measured I+II is fast, shallow and approximately mono. A lush parallel mode is a separate enhancement.
- Physical left footswitch is the effect action; physical right is bypass. The current public SDK exposes left press and hold only, with no release or global routing query.
- Keep original dry channels available when designing stereo routing. Do not infer cable configuration by looking for silence.

Use const and camelCase where appropriate; preserve SDK names and existing ABI conventions. Prefer named module-level functions and avoid `++` / `--` in new code. Avoid em dashes in prose. Check applicable CLion inspections when the connector is available and report when it is not.

## Pass-through diagnostic

The active passthrough implementation is `effects/passthrough/`; see its README and the preserved `example/diagnostic/README.md` for REW procedures. It uses the same pinned SDK while excluding the chorus implementation. Use `make build EFFECT=passthrough` or the legacy diagnostic Makefile. Preserve both audio channels exactly; hardware validation is separate from host sample-preservation tests.

## Temporary chorus Mix mapping

The 2026-09-28 test change preserves original stereo dry samples and maps 0-5%
to true dry, noon to 50/50, and maximum to fully wet. See `effects/junoChorus/README.md`
for validation/build commands and the separate final-product control preference.
Always supply exact REW measurement names when requesting captures from Ryan.

Ryan authorizes local commits at applicable milestones. Keep unrelated edits out of commits and do not infer push authorization. After loading a test binary, sweep each knob and set its requested position before recording.

When a measurement handoff is reached, always provide the next concrete steps and exact test names without waiting for Ryan to ask. Continue authorized steps autonomously; wait for confirmation of required physical wiring or pedal changes.

For pedal capture work, Ryan prefers normal-use pedal Input/Output settings left unchanged. Do not request secondary-gain adjustments as a routine prerequisite. He reports the measured harmonics are not audible; distinguish measurements from demonstrated listening problems. Vary the interface/REAPER send for controlled characterization instead.

## Vintage candidate (2026-09-29)

The approved candidate now supersedes the experimental wet algorithm. Read
`docs/vintage-candidate-20260929.md` for exact build identity, motion, filters,
headroom allowance and next capture names. Both host tests and ARM build pass;
hardware validation is pending. Preserve diagnostic Mix until those captures
are complete. Do not describe the provisional wet EQ or gain as circuit-exact.

## Multi-effect and SDK maintenance (2026-09-30)

Read `docs/effect-authoring.md`, `docs/sdk-maintenance.md` and `docs/audio-workflow.md` before adding effects or updating the SDK. Check upstream through `scripts/sdk.py check`; explicit reviewed imports use `update --ref <fullCommit> --apply`. Keep fork-only APIs out of the official contract. Use templates under `docs/templates/` for decision and hardware evidence records. Capture both channels and retain raw gain/timing. The relocation preserves Juno samples; it does not resolve outstanding pedal validation. Desktop audition now has a project-owned JUCE VST3 wrapper; see `desktop/README.md`.

Desktop prerequisites were installed on 2026-09-30: CMake 4.4.3 and a local JUCE 8.0.4 checkout under ignored `build/desktop/dependencies/JUCE/`. `desktop.lock.json` pins its full commit. Read the dependency setup section in `docs/desktop-audition-evaluation.md` and build with `python3 scripts/desktop.py test`. Native arm64 Juno VST3 was installed and loaded in REAPER 7.80 at stereo 48 kHz. Separate instance factories retain the official SDK ABI; prepare resets DSP history while preset loading updates controls. Desktop validation does not establish pedal CPU performance.
