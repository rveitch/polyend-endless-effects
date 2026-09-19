# Juno chorus completion plan

Prepared 2026-09-19. **Status: proposed, not implemented.** This is a staged engineering plan with research decision gates, not a claim that all analog calibration values are already known.

**Goal:** deliver a reliable Endless effect that reproduces the chosen Juno-60 reference behavior and preserves Ryan's simple control layout.

**Architecture:** a small platform-independent chorus core plus a thin Endless adapter. Keep delay interpolation, modulation, wet-path conditioning and final routing separately testable. Start with the simplest measured model and add clock-driven BBD behavior only when reference comparison demonstrates a worthwhile improvement.

**Stack:** C++20, 48 kHz float processing, existing Cortex-M7 ARM GNU build, standalone host tests. No new package manager or DSP framework is required for the baseline.

**Design evidence:** [hardware research](research/juno-chorus-evidence.md), [reference implementations](research/reference-implementations.md), [platform contract](platform/polyend-endless.md), [historical preferences](research/history-and-decisions.md), [audit](code-audit.md), [validation protocol](validation.md).

## Scope and recommendation

| Approach | Benefits | Tradeoffs |
| --- | --- | --- |
| Measured delay/filter baseline, then calibration | Small, testable, fits current code and allows early hardware comparison | Initially omits some BBD clock and analog detail |
| Tune the existing lush experimental model against plugins | Fast path to a preferred sound | Does not establish hardware authenticity, especially I+II |
| Full clock-driven BBD/circuit model immediately | Can capture sample-clock and analog-filter interactions | More DSP complexity and target profiling before benefit is known |

**Recommendation:** first approach. Preserve the present experimental revision as a reference before replacing its behavior. If Ryan prefers a particular plugin as the target, record that explicitly and adjust the mode table rather than labeling plugin-specific behavior as a circuit fact.

## Global constraints

- Keep the Endless ABI, 48 kHz sample rate and no-allocation target callback contract.
- Keep the existing Arm GNU toolchain and narrow repository ownership.
- Preserve the 0–95% mix plateau and 40–60% neutral tone plateau unless Ryan revises them.
- Physical left action: short toggles I/II; hold latches I+II while no release API exists. Verify actual gesture event order before finalizing state transitions.
- Physical right bypass and firmware-level secondary controls remain owned by the platform.
- Select and document stereo input behavior and global Mix settings before final comparison.
- Noise, saturation, width enhancement and parallel I+II must have evidence or an explicit extension label.
- No deployment or firmware changes are needed for the documentation phase. Device replacement tests belong to the implementation/validation phase.

## Review focus

1. Correlated or anti-phase stereo input must follow a documented routing policy.
2. Full wet plus maximum width must not clip valid normalized input.
3. Both-button mode must match the selected hardware or plugin target, including stereo relationship.
4. Short/hold event sequences and arbitrary callback boundaries must not cause mode surprises or clicks.
5. Header edits and saved binaries must not create false evidence of a current successful build.

## Stage 1: restore one buildable, traceable baseline

**Files:** `example/source/PatchImpl.cpp`, `example/Makefile`, planned `tests/Makefile` and `tests/chorus-baseline.cpp`.

- [ ] Save provenance of the currently used `.endl` and record its hash. Do not assume it came from the duplicated commit.
- [ ] Remove the appended-definition conflict by retaining one complete implementation. For the initial repair, retain the later block as the documented v2 candidate without also retuning it. Git history preserves the earlier block.
- [ ] Run a fresh ARM build under existing warning flags. A fresh `BUILD_DIR` prevents stale objects; inspect ELF size and resulting `.endl`.
- [ ] Add a finite host smoke test that constructs a fresh core or patch, supplies workspace, runs fixed input and checks finite output. Add failing regression cases for nominal five-sample delay and full-width 320 Hz overshoot before fixing them in Stage 2.
- [ ] Make builds track included headers (`-MMD -MP` and included dependency files) and verify that touching the relevant header recompiles its object. Keep release naming explicit.
- [ ] Record the build repair separately from the DSP changes so it can be reviewed independently.

**Acceptance:** one singleton definition, fresh ARM build succeeds, current artifact identity is recorded, failing DSP regression cases reproduce the audit. A successful build is not yet a release.

## Stage 2: correct delay, routing and gain contracts

**Files:** split focused core into proposed `example/source/JunoChorus.h` and `JunoChorus.cpp`; retain `PatchImpl.cpp` as SDK adapter. Add `tests/delay-line.cpp`, `tests/chorus-routing.cpp`, `tests/chorus-levels.cpp` and host build targets. Do not change SDK glue unless a specific proven contract requires it.

**Boundary:** core receives two audio spans, working storage and normalized controls; adapter owns SDK names, action translation and LED colors. Core owns DSP state. Implementer defines this small interface before splitting and keeps the same interface across subsequent stages.

- [ ] Pin integer and fractional delay convention with impulse tests, including wraparound. Fix the current one-sample error.
- [ ] Keep original left/right samples available before wet excitation. Recommended pedal policy is preserved stereo dry with mono-summed wet input, subject to Ryan's preference and firmware routing test. Explicitly document the wet cancellation tradeoff for anti-phase input.
- [ ] Move optional noise and wet coloration into their intended branch. Begin structural tests with noise disabled; do not alter measured filter response merely to make an effect seem louder.
- [ ] Establish a gain budget covering wet makeup, width and final mix. Re-run the actual 0.99-amplitude sine regression across frequencies, modes and width endpoints. Avoid using uncalibrated hard clipping as the only solution.
- [ ] Clamp or reject invalid control values using a defined finite fallback, while preserving the requested valid-range plateaus.
- [ ] Verify callback-partition invariance, fresh-state silence, left-only, right-only and anti-phase inputs under the chosen policy.

**Acceptance:** delay impulse tests pass; normalized stress inputs remain strictly inside output bounds; dry/stereo behavior matches the written policy; block partition tests pass. ARM build still succeeds with the same ABI.

## Stage 3: implement the selected Juno mode model

**Files:** `JunoChorus.h/.cpp`, proposed `tests/chorus-modes.cpp`, and a documented calibration table in `docs/research/calibration.md`.

- [ ] Choose one initial measurement dataset. Recommended: Hera's attributed per-channel ranges, cross-checked against the recording analysis, rather than mixing minima from different studies.
- [ ] Implement shared opposing modulation for I/II at approximately 0.513/0.863 Hz. Use the selected left/right delay ranges consistently and retain their provenance.
- [ ] Implement hardware I+II as common, fast, shallow modulation around 9.75 Hz using the selected per-channel ranges and rounded/sine-like trajectory. Do not approximate it as 20% of an unrelated normal-mode depth; use measured absolute ranges.
- [ ] Measure rate, minimum/maximum delay and left/right correlation over at least ten slow-mode cycles. Compare those trajectories before judging the audio by preference.
- [ ] Smooth control and mode transitions. Test switches at several phase positions; choose a documented ramp/crossfade duration from click measurements and listening, not an unexplained constant.
- [ ] If a lush parallel variant is wanted, keep it separately named and tested. With only three knobs and press/hold, choose its access method explicitly; do not overload the existing gesture invisibly.

**Acceptance:** measured software trajectories match the chosen dataset within declared numerical tolerances, all three hardware modes have the correct relationship, and mode transition tests pass. Parameter variation between historical units remains documented.

## Stage 4: calibrate filtering, level and optional analog character

**Files:** core/filter implementation as needed, `tests/chorus-response.cpp`, `docs/research/calibration.md`, and reference-capture metadata under proposed `tests/fixtures/README.md`.

- [ ] Capture or obtain properly documented reference audio. Use Ryan's available plugins as listening references, with exact names and versions; use hardware recordings or circuit derivation for hardware claims.
- [ ] Measure wet-path response and dry/wet gain separately. Start from the actual schematic and source research; a summing-resistor ratio alone is not the whole transfer function.
- [ ] Fit the neutral filter response at the tone plateau. Test magnitude and phase on fixed-delay conditions before re-enabling modulation.
- [ ] Compare ordinary fractional delay against a clock-history-aware model. Prototype detailed BBD processing on the host only if the simpler model's residual difference warrants it.
- [ ] If extracting Hera, Junologue or TAL-derived code, inspect the exact files and dependencies and preserve the applicable notices. TAL-NoiseMaker-derived source is not evidence of current Chorus-LX internals.
- [ ] Calibrate nonlinearity at several levels and noise by spectrum/RMS. Keep these optional until their benefit is demonstrated. Do not add a compander without Juno-specific evidence.
- [ ] Recheck tone and width extremes against gain, mono-sum and listening tests after every voicing change.

**Acceptance:** calibration report identifies the sources, test conditions, residual differences and chosen complexity. A simpler model may ship if it meets the agreed audible target; do not claim full circuit equivalence without that evidence.

## Stage 5: integrate and verify on Endless

**Files:** `PatchImpl.cpp`, hardware test notes in `docs/validation-results.md`, any focused adapter tests.

- [ ] Observe mono/stereo startup routing and global Mix interaction using known signals. Record the reference settings.
- [ ] Verify short/hold event order, latch behavior and LED mapping. Do not implement momentary return without a release event.
- [ ] Test expression, rapid parameter motion, bypass entry/exit and tail behavior with the actual firmware.
- [ ] Profile the worst processing path on target where possible. Avoid claiming CPU margin from host timings or the 720 MHz headline specification.
- [ ] Run the sustained audio test in the validation protocol and listen for clicks, dropouts and overload at realistic input levels.

**Acceptance:** the documented control behavior works on the pedal, hardware stress/listening tests pass, and any remaining firmware uncertainty is explicit.

## Stage 6: deliver a reproducible release

**Files:** `README.md`, `example/README.md`, release notes and build instructions; named artifact plus ELF retained in the agreed release location.

- [ ] Produce a fresh named `.endl` using the final source and record hash, commit and compiler version.
- [ ] Include control chart, correct physical footswitch labels, startup routing and global Mix setting, installation instructions and known differences from reference hardware.
- [ ] Record which checks passed and which require future evidence. Include approved listening references and residual differences.
- [ ] Update `AGENTS.md` to remove the resolved duplicate-build warning and point to actual tests and calibration results.
- [ ] Have Ryan accept the listening result before labeling the emulation finished. Commit/publish through the authorized repository workflow when requested.

**Definition of done:** reproducible target build, passing DSP regressions, documented reference calibration, verified pedal controls/routing/performance, and Ryan's listening acceptance. This review completes the research/documentation phase only.
