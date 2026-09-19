# Current implementation audit

Verified 2026-09-19 at effects commit `bc31ea43a596f6f19fcffdaf1db41efd5614376e`. Line references describe that baseline; refresh them after edits. The complete 600-line implementation was available to inspect. No DSP source was changed during this audit.

## 1. Build blocker: two complete implementations

`example/source/PatchImpl.cpp:1–289` contains the original implementation. Lines `290–600` repeat the include, `DelayLine`, `OnePoleLP`, `PatchImpl`, static `patch` and `Patch::getInstance()` definitions.

The April 2 v2 commit appended a new version rather than replacing the old one. A real ARM target build failed with redefinitions at lines 313, 363, 382, 595 and 597. This is the first repair required before producing any new effect binary.

Do not assume an existing `.endl` represents this commit. Record hashes and provenance of any currently installed or saved effect before comparing sound.

## 2. Reproduced output overshoot

Lines `482–487` apply `tanh(wet * 1.2) * 1.15`, then widen the stereo side component. `getStereoAmount(1)` returns 1.25 at `554–566`. Saturation before that gain operation does not bound the final output.

A temporary host diagnostic instantiated a fresh second-version `PatchImpl` for each case, supplied its working buffer, set full wet and full width, and processed one second of identical left/right 0.99-amplitude sine input at 48 kHz. It excluded the first 1,024 output samples when measuring peaks:

| Input frequency | Output absolute peak |
| --- | --- |
| 80 Hz | 1.00079 |
| 160 Hz | 1.14177 |
| 320 Hz | 1.19252 |
| 640 Hz | 1.19100 |
| 1 kHz | 1.18749 |
| 2 kHz | 1.17075 |

This violates the SDK output-range guidance for valid normalized input. A constant input did not reveal the problem because its delayed channels converge. Test stereo-difference peaks, not only DC. Fix the gain budget and width mapping before choosing a final protective output stage; arbitrary hard clipping could conceal the original gain error.

## 3. Dry signal and stereo information are not preserved

At `442–444`, both input channels are averaged, noise is added, and the result is low-pass filtered at 12 kHz. Lines `489–490` use that processed mono value as the dry contribution as well as feeding both delay paths.

Consequences:

- Anti-phase stereo content cancels even in the nominal dry contribution. With `L=0.5`, `R=-0.5`, the isolated revision produced only the injected noise, RMS approximately `1.61e-5`.
- Silence is not silent. The current arbitrary noise reaches both dry and wet paths.
- The pedal's original stereo image cannot survive this topology.
- A mono signal supplied only to the left span would lose 6 dB at the summing point. Whether firmware duplicates mono input is unknown, so this is a routing question to test, not a proven device loss.

The original Juno receives a mono source, so mono wet excitation is not inherently wrong. The pedal adaptation needs an explicit stereo policy and separate dry/wet routing. Do not change it based solely on assumptions about global hardware Mix.

## 4. Delay indexing is one sample short

`DelayLine::write()` at `330–335` advances the write pointer. `read()` at `337–351` then subtracts the requested delay from that already-advanced pointer. The processing loop writes before reading at `473–477`.

An isolated unit diagnostic wrote an impulse, then read with a constant delay of five samples on every iteration. The nonzero output occurred at index **4**, not 5. This is a small audible timing difference at 48 kHz, but a clear correctness error that will corrupt calibration and fractional-delay tests. Establish one read/write convention and test integer and fractional impulse positions.

## 5. Mode behavior and calibration

The later block uses:

| Mode | Center | Depth | Nominal range | Modulation |
| --- | --- | --- | --- | --- |
| I | 3.70 ms | ±0.95 ms | 2.75–4.65 ms | 0.513 Hz triangle |
| II | 3.35 ms | ±1.45 ms | 1.90–4.80 ms | 0.863 Hz triangle |
| I+II | 3.50 ms | ±1.25 ms | 2.25–4.75 ms maximum envelope | 60/40 blend of I and II |

The 1.66–5.35 ms constants at `416–417` only clamp the delay; they do not make either mode traverse that measured range. The blended mode's realized trajectory depends on relative phase and is not a single periodic 9.75 Hz shallow sweep.

For I+II, line `471` retains inverted left/right modulation. The measured hardware both-button mode is approximately mono and uses a much shallower, faster modulation. Both the trajectory and stereo behavior need revisiting for hardware fidelity. See [research evidence](research/juno-chorus-evidence.md).

At `446–464`, phase I advances continuously but phase II stops while I is selected. Switching modes can jump to a previously frozen modulation position. Parameter and mode changes are applied without ramps at callback boundaries. Clicks have not been measured on the pedal, but transition continuity should be tested before release.

## 6. Controls and voicing

- The mix plateau at `531–535` matches Ryan's stated preference. It is **not a defect**. No fully dry knob position was requested; hardware bypass/global Mix are separate controls.
- Tone is flat over 40–60% rotation at 8.5 kHz, with 4.5–12 kHz one-pole cutoff elsewhere. That plateau also matches the preference. Its claim to authentic neutral voicing has not been calibrated.
- The input and output one-pole filters, tanh transfer, gain and noise have no circuit-derived fit in the current repository.
- Width is unity over roughly the center plateau, but 1.25 at maximum. It changes wet mid/side gain, not LFO polarity in this later revision.
- Short action clears I+II and toggles the base mode; hold toggles a latch (`506–516`). There is no release handler in the SDK. Whether firmware emits press before hold must be observed before assuming a hold preserves the intended base mode.
- `setParamValue` does not clamp values. Valid SDK metadata values are the primary contract; defensive finite-range handling is reasonable for tests, but not evidence of a current firmware error.

## 7. Lifecycle, memory and performance

The patch uses 2,048 floats for two 1,024-sample buffers, about 8 KiB of the supplied 9.6 MB float workspace. Capacity easily covers the current delay range. Linear interpolation and duplicate storage are simple; neither should be replaced before measurement shows a benefit.

The SDK says `init()` and buffer assignment are called once at patch load. `init()` does not clear filter states, so repeatedly calling it on the singleton in a host harness is not a fresh reset. Create a fresh instance per independent host case. Treat reinitialization without a reload as an unproven host scenario, not a confirmed product bug.

`DelayLine::write()` and `read()` check for a null buffer. The initial audit's suspicion of unconditional modulo-by-zero before buffer assignment does not apply to these guarded methods. Do not include it as a finding.

The SDK guarantees equal audio-span sizes. Null or mismatched spans in custom wrapper calls are outside the documented patch contract. Broad defensive wrapper rewrites are not a priority for this effect review.

Three exponentials are recomputed per audio callback and two tanh operations occur per sample. Actual CPU cost depends on target math-library code and callback size. No pedal profile was performed. The existing compiler builds the upstream SDK example successfully.

## Diagnostic limits

The host checks extracted only lines 290–600 into a temporary file. They demonstrate behavior of that revision, not a successful build of the real duplicated file. No listening result, latency figure, CPU margin or hardware equivalence is claimed.

Commands used: C++20 host compilation of temporary harnesses with `example/source` as an include directory; real target `make` with the existing ARM compiler and a temporary build directory. Temporary harnesses were `/tmp/juno_host_diag.cpp` and `/tmp/polyend-width-diagnostic.cpp`. The durable follow-up test cases are specified in [validation](validation.md).
