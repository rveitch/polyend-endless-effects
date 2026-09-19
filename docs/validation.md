# Validation and reference capture protocol

Proposed protocol, 2026-09-19. Thresholds below are project acceptance criteria, not manufacturer specifications. No hardware or plugin captures have been made in this review.

## Capture conditions

Record effect commit, binary SHA-256, compiler version, device firmware if discoverable, input mode, cables, global Input/Output/Mix settings, all patch knob positions, reference plugin name/version, sample rate and interface gains. Use identical input WAVs and fixed seeds for offline comparisons. Preserve raw captures before loudness matching.

Use 48 kHz source files. A reference recording should span at least ten cycles of the slowest mode, about 20 seconds, after a defined settling interval. Use a longer segment if the alternate dual-rate mode requires it. Record bypass, I, II and both-button modes separately.

Disable optional plugin noise and extra stereo processing for the first structural comparison. Then compare those features separately. Do not normalize away gain or noise errors in objective tests; level-match only the listening copies and record the gain adjustment.

## Host tests before target deployment

Create fresh DSP instances per case. Keep heap allocation in the host harness, not in the target processing path. Process each input with several partitions, including 1, 16, 64 and 127 samples plus an incomplete final block. The SDK does not guarantee those exact block sizes; varying partitions detects accidental callback-size dependence.

| Test | Input and procedure | Proposed pass condition |
| --- | --- | --- |
| Delay indexing | Impulse into constant 5 and 5.5 sample delays | Index 5 at amplitude 1 for integer; expected adjacent 0.5 weights for linear fractional interpolation |
| Delay wrap | Repeated impulses around ring boundary; minimum and maximum delay | No out-of-bounds access, missing impulse or discontinuity |
| Clean silence | Zero input with noise disabled | Finite output, below `1e-7` after settling |
| Gain/headroom | 0.99-peak sines at 80/160/320/640/1000/2000 Hz, chirp, square, impulse, seeded noise; all modes and control endpoints | No NaN/Inf and peak strictly below 1, with documented margin |
| Routing | Left only, right only, identical L/R, anti-phase L/R | Matches explicitly selected stereo policy; original dry channels are retained when that policy requires it |
| Controls | Mix at 0/.5/.95/1; tone at .39/.4/.5/.6/.61; width endpoints | Exact documented plateaus and continuous boundary mapping |
| Mode identity | Extract delay trajectories over multiple cycles | Correct rate, range, relative phase and both-button stereo behavior against chosen calibration table |
| Block partitioning | Identical signal/control schedule under several block sizes | Equivalent output within floating-point tolerance, proposed `1e-6` maximum absolute error |
| Mode transitions | Switch at multiple LFO phases; rapid short/hold patterns | No abrupt delay jump by construction; transition residual measured and listened to |
| Invalid controls | Out-of-range, NaN, unknown IDs in host calls | Chosen defensive policy, finite output, no memory error |

For a minimal standalone delay test, the required convention is:

```cpp
// Pseudocode for the planned DelayLine contract, not an existing API guarantee.
for (int sample = 0; sample < 12; sample += 1) {
    line.write(sample == 0 ? 1.0f : 0.0f);
    const float actual = line.read(5.0f);
    const float expected = sample == 5 ? 1.0f : 0.0f;
    assert(std::abs(actual - expected) < 1.0e-6f);
}
```

Noise-enabled tests should measure RMS and spectrum against a chosen source recording rather than merely expecting nonzero noise. Test filter magnitude and phase separately from the time-varying delay. A static-delay mode in the test harness helps separate interpolation coloration from filter coloration.

## Hardware contract checks

1. Record mono-startup and stereo-startup routing with left-only and right-only test inputs. Determine what samples actually reach each patch channel.
2. Sweep global Mix with a simple known patch to establish dry/wet law, gain and delay. Set a documented reference position, preferably full patch output, for emulation comparison. Do not assume two 50% mixes mean 50% overall effect.
3. Observe short and hold event sequences. Determine whether a hold also causes a press, and what happens on release. Use LED state or a dedicated diagnostic patch only when authorized to replace the installed effect.
4. Verify LEDs, expression control, startup defaults, bypass transitions and tails.
5. Measure callback timing if the platform exposes a safe method; otherwise document the measurement limit and run a sustained worst-case audio test. Host execution time cannot establish Cortex-M7 CPU margin.

## Listening and completion

Compare sustained bright synth tones, bass, chords, transients and the intended real instruments. Listen in stereo, each output individually, and mono sum. Compare I/II motion, I+II fast shallow character, wet-only pitch modulation, bandwidth, level, low-frequency retention and noise separately.

A subjective preference for a lush parallel mode is valid, but does not disprove a fast hardware mode. Save both variants with honest names if both are kept.

Release requires: clean target build, passing host tests, documented device control/routing behavior, no audible glitches in a proposed 30-minute worst-case soak, reference listening accepted by Ryan, known global settings, and a named `.endl` plus retained ELF, hash and build metadata. Do not call an offline render hardware-validated.
