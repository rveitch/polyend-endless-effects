# Vintage candidate implementation and pedal handoff

Date: 2026-09-29. Branch: `fix/chorus-build`. Implements the approved
[DSP proposal](dsp-revision-proposal-20260929.md). This is a test candidate,
not a circuit-exact model or a hardware-validated release.

## Build identity

- Artifact: `example/build-vintage-candidate-final-20260929/juno_vintage_candidate_20260929_131540.endl`
- SHA-256: `8283b522be060389e31964c6ba8a3b9d444967b7a22d6f9330d8d8ac64596fa4`
- Source SHA-256 (`example/source/PatchImpl.cpp`): `ed8a6735daf91e8cafc8dc26e49eb052a478b9646f525580e9d5b57e279ff180`
- ARM GCC 15.2.1, Cortex-M7 single-precision hard-float, SDK flags unchanged.
- ELF size: text 11596, data 0, bss 5 bytes. This does not describe total firmware or externally provided working-buffer memory.
- Existing binaries preserved. No pedal deployment or push performed.

```sh
make -C example TOOLCHAIN=/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi- BUILD_DIR=build-vintage-candidate-final-20260929 PATCH_NAME=juno_vintage_candidate all
```

Use a new build directory for subsequent source revisions. The SDK Makefile
has incomplete header dependencies. This exact artifact is the handoff build;
the earlier `build-vintage-candidate-20260929` directory is superseded.

## Implemented behavior

| Mode | Rate | Delay range | Stereo wet relationship |
| --- | --- | --- | --- |
| I, red | approximately 0.513 Hz | 1.66 to 5.35 ms | Opposing triangle modulation |
| II, green | approximately 0.863 Hz | 1.66 to 5.35 ms | Opposing triangle modulation |
| I+II, blue | approximately 9.75 Hz | 3.3 to 3.7 ms | Common sine modulation |

The unsigned fixed-point phase clock produces actual rates 0.512994826,
0.863000751 and 9.749997407 Hz at 48 kHz. All clocks, taps and filters keep
running in every mode, including dry. Mode changes crossfade over 960 samples
(20 ms), starting from current weights when interrupted. The delay reader's
one-sample indexing error is corrected.

Wet excitation is the average of the two inputs. Original independent stereo
dry samples are preserved. The wet path uses second-order Butterworth
low-pass filters before the delay at 8 kHz and after it at 4.5 kHz. Tone
40 to 60% is neutral; its endpoints give postfilter cutoffs 3.6 and 5.4 kHz.
Cutoff coefficient changes ramp over 20 ms. This is a provisional tonal match,
not a claim about the original circuit's exact transfer function.

Noise injection, arbitrary saturation and makeup gain are removed. Wet gain
is 0.7 (approximately -3.1 dB), a deliberate deviation from unity starting
gain to allow filter overshoot without clipping. This is a headroom budget,
not an original-Juno gain measurement. Do not normalize away this difference
in raw captures. Width runs from mono at minimum to nominal stereo at 40%
and stays nominal above that point. It no longer boosts the side signal.

Diagnostic Mix mapping remains: 0 to 5% exact dry, noon 50/50, maximum fully
wet. Mix and Width changes are immediate; do not claim all controls are
smoothed. Final mostly-static Mix controls remain deferred. Nonfinite knob
updates are ignored and finite values are clamped. Footswitch actions and
LED assignments are unchanged.

## Verification

Both host test programs pass with `-O2 -Wall -Wextra -Werror` and UBSan.

- Existing Mix tests: exact dry, dead-zone boundaries, independent stereo
  samples, continuous blend mapping, evolving wet state.
- Candidate tests: integer/fractional delay impulses including ring wrap;
  ten-second rate counts and delay extrema; opposing normal-mode and common
  combined-mode relationships; silent input has no injected noise.
- Mode fades agree with continuously running pure-mode renders, including
  a fade interrupted after 300 of 960 samples and its final endpoint.
- Results are bit-identical across 64- and 8-sample callbacks during repeated
  mode and Tone changes, including changes before ramp completion.
- Near-full-scale square-wave stress at 80, 320, 1000, 5000 and 18000 Hz with
  changing mode, Tone and Width remains finite; observed peak 0.791726.
- Neutral fully wet combined-mode sine RMS response relative to 1 kHz:
  5 kHz -4.89 dB, 10 kHz -23.65 dB. Both meet the proposal's ranges.
- Fixed-coefficient impulse absolute-sum estimate across the Tone range:
  prefilter 1.13720, maximum postfilter 1.10956, product times wet gain
  approximately 0.88326. This is not a bound for time-varying coefficients;
  transition stress covers only the tested signals and changes.
- ARM build passes. Existing SDK RWX LOAD-segment warning remains.
  Final binary contains no matching ARM software-double helper symbols.

ASan could not initialize on this host (sanitizer_malloc_mac.inc assertion),
so no ASan coverage is claimed. The IDE connector did not expose Polyend;
no CLion inspection result is claimed. Hardware CPU usage, clicks during
physical controls, detailed phase/group-delay response and broadband
modulation behavior remain pedal-validation tasks. Host tests establish
candidate behavior, not authenticity or listening quality.

## Next physical steps and capture names

1. Load the exact artifact above.
2. While engaged, sweep all three knobs once. Set Mix fully counterclockwise,
   Tone and Width to noon; select Mode I (red). Then bypass the pedal.
3. Keep established wiring and global Input/Output levels unchanged. Retain
   the documented capture-session global Mix state; record any reset or
   other change rather than silently treating earlier captures as comparable.
4. Tell the capture operator the pedal is ready in bypass. Record the baseline,
   then request engagement for true-dry comparison before fully wet testing.

Use these exact names, appending `take2` in place of `take1` for repeats:

- `Endless vintageCandidate I bypass broadband peakMinus45 stereo 48k take1`
- `Endless vintageCandidate I dry0 tone50 width50 broadband peakMinus45 stereo 48k take1`
- `Endless vintageCandidate I wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`
- `Endless vintageCandidate II wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`
- `Endless vintageCandidate IplusII wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`

These names specify the intended stimulus peak, not a measured return level.
Verify the actual send before recording. Use the established simultaneous
ADAT3 loopback timing reference and preserve raw timing and gain. Confirm
required pedal states with Ryan before each capture. Start with bypass/dry
agreement, then measure fully wet motion/coloration, followed by matched-level
listening. Do not change global pedal gains to compensate for candidate wet gain.

## First candidate bypass capture

User confirmed candidate loaded, knobs swept and bypass active, with global Mix
still 100%. Reopened the saved hardware capture session and added the original
34-second broadband stimulus at timeline position 1700, unity item/track gain.
Saved routing: mono hardware outputs 11/13, stereo input 11/12 and mono reference
13, monitoring and master sends off on the recording tracks. Existing takes
were preserved. The MCP Record helper failed with `Unknown function:
OnRecordButton`; native REAPER action 1013 started recording successfully.
Transport was stopped and the project saved after capture.

`vintageCandidate-bypass-analysis.json` in the capture directory records full
paths, hashes and methods. Both raw recordings contain 2040784 frames at 48 kHz
(longer than the 34-second punch selection; retain these originals). Analysis
uses samples from 3 to 31 seconds, without gain or timing normalization.

- Reference RMS: -59.44845 dBFS.
- Pedal L/R RMS: -58.66971 / -58.70224 dBFS.
- L/R peak: -44.10742 / -44.13554 dBFS; zero full-scale samples.
- L/R relative correlation delay: approximately 415.01 samples, 8.646 ms.
  This is a broadband correlation estimate, not an absolute interface delay
  or a replacement for the earlier REW phase/timing estimate.
- L/R correlation: 0.9999986, consistent with the bypassed mono-input setup.

Next: user engages Mode I/red with patch Mix minimum, Tone/Width noon and global
Mix unchanged. Capture the named dry0 take before drawing dry/bypass conclusions.
User noted a new upstream MCP release; investigate after the matching dry take
so bridge changes do not interrupt this comparison.

## Engaged dry comparison

User confirmed Mode I/red engaged, with requested Mix minimum and other settings
unchanged. Recorded the same source at timeline 1750..1784 using native Record
action 1013. Transport stopped and session saved. Raw WAV paths/hashes and
analysis are in `vintageCandidate-dry0-analysis.json` beside the REAPER project.

- Dry RMS L/R: -58.66978 / -58.70231 dBFS.
- Per-take reference-normalized level change versus bypass: -0.000024 /
  -0.000032 dB L/R. Reference RMS remains approximately -59.44849 dBFS.
- Relative correlation delay remains approximately 8.646 ms on both channels;
  do not interpret sub-sample estimator differences as physical timing precision.
- Maximum Welch power-ratio response difference from bypass, 40 Hz to 18 kHz:
  0.00427 / 0.00442 dB L/R, using 16384-sample segments and raw 3..31 seconds.
- No full-scale samples. This supports dry/bypass agreement at this signal
  level with mono input; it does not establish stereo-input independence or
  wet-mode behavior.

Next requested state: engaged Mode I/red, patch Mix maximum, Tone/Width noon,
global Mix 100%, global Input/Output unchanged. Use the wet100 name above.

Checked upstream TwelveTake release v1.7.8, published 2026-09-29 11:30 UTC:
https://github.com/TwelveTake-Studios/reaper-mcp/releases/tag/v1.7.8
Release notes cover bridge reload/autostart and timing-signature fixes, but do
not list the failed OnRecordButton helper as fixed. No update installed during
these captures; native Record works. Initial migration requires bridge redeploy
and manual script restart according to the release notes.

## Mode I fully wet capture

User's proceed response was interpreted as confirmation of the requested red
Mode I, patch Mix fully clockwise, Tone/Width noon, global Mix 100%, and unchanged
Input/Output. Capture at timeline 1800..1834 uses the same source and routing.
Native Record action succeeded; transport stopped and project saved afterward.

Evidence: `vintageCandidate-I-wet100-analysis.json` in the capture directory
contains raw filenames, hashes, full delay trajectories and analysis method.
Existing `Plugin References/analyzeModeI.py` readAudio, bandLevels and trajectory
helpers were reused without running its top-level reference-analysis job.

- Reference RMS: -59.44850 dBFS; pedal L/R RMS: -68.33882 / -68.34227 dBFS.
- Peak L/R: -54.13834 / -54.05282 dBFS; no full-scale samples.
- Fitted modulation fundamental: 0.51298 Hz on both channels.
- L/R delay-trajectory correlation: -0.9999988, consistent with opposing motion.
- Apparent wet delay after subtracting the bypass correlation estimate:
  L 1.810..5.407 ms, R 1.754..5.354 ms. These estimates include wet filter
  group delay and finite-window correlation bias; do not use their extrema
  as exact delay-line bounds. Median local correlation is about 0.51 because
  the wet signal is filtered and time-varying.
- Bypass-corrected 1 kHz band gain: -3.132 / -3.117 dB L/R, consistent with
  the provisional 0.7 wet gain.
- Relative to 1 kHz, third-octave band levels: 5 kHz -4.930 / -4.908 dB;
  10 kHz -22.987 / -22.973 dB. These support the proposed coloration range.
  The 18 kHz band is approximately -55.5 dB relative to 1 kHz but is not
  treated as a precise filter measurement at this low capture level.
- Broadband L/R audio correlation: 0.00624.

Measurements support intended Mode I motion and coloration, not authenticity,
absence of every possible glitch, or final listening approval. Next: select
Mode II/green while retaining both Mix controls fully wet and Tone/Width noon.
Capture `Endless vintageCandidate II wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`.

## Mode II fully wet capture

User confirmed green Mode II; all requested knob/global settings unchanged.
Recorded at timeline 1850..1884 with native Record, then stopped and saved.
`vintageCandidate-II-wet100-analysis.json` beside the session stores filenames,
hashes and full trajectories. Same Mode I analysis helper definitions, with
fundamental search expanded to 0.4..1.3 Hz to include Mode II.

- Fitted rate: 0.862965 / 0.862963 Hz L/R, matching the 0.863 Hz candidate.
- Delay-trajectory correlation: -0.9999909, opposing stereo modulation.
- Apparent wet-delay extrema after bypass subtraction: approximately
  1.769..5.392 ms L, 1.771..5.393 ms R. Filter delay and finite-window bias
  remain included; these are not exact delay-line endpoints. Median local
  correlations around 0.395 are affected by filtering and delay movement.
- Bypass-corrected 1 kHz band gain: -3.144 / -3.102 dB L/R.
- Relative to 1 kHz: 5 kHz -4.924 / -4.919 dB; 10 kHz -22.974 / -22.985 dB.
  Coloration agrees closely with Mode I.
- Reference RMS -59.44843 dBFS; output RMS -68.33877 / -68.34240 dBFS.
- Peaks -54.09836 / -54.05595 dBFS; no full-scale samples.
- Audio L/R correlation 0.01083. These results support intended Mode II
  behavior at this test level, not a complete hardware performance approval.

Next: select I+II/blue, retain patch/global Mix fully wet and Tone/Width noon.
Capture `Endless vintageCandidate IplusII wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`.
For the fast mode, increase trajectory sampling frequency above the previous
20 Hz and fit near 9.75 Hz; do not reuse the slow-mode frequency search.

## I+II fully wet capture and first wet-mode milestone

User confirmed blue I+II, keeping both Mix controls fully wet and Tone/Width
noon. Recorded at timeline 1900..1934 using the same stimulus/routing, then
stopped and saved. `vintageCandidate-IplusII-wet100-analysis.json` beside the
session includes raw filenames/hashes and full delay trajectories.

Analysis reuses the Mode I helper definitions with 512-sample correlation
windows at 100 Hz and a 5..14 Hz fundamental search. This avoids the slow-mode
20 Hz trajectory sampling limit. Sine center/depth estimated by least squares.

- Fitted rate 9.74970 Hz on both channels; fitted depth +/-0.19735 ms.
- Apparent center 3.5767 ms, extrema approximately 3.377..3.772 ms after
  subtracting bypass delay. Includes wet-filter group delay and estimator
  bias; do not interpret as a 0.077 ms delay-line programming error.
- Sine-fit residual approximately 0.00503 ms; median local correlation 0.488.
- Delay-trajectory L/R correlation 0.99999993 and audio correlation 0.99999167,
  supporting the intended common wet modulation/output in this setup.
- Bypass-corrected 1 kHz band gain -3.133 / -3.104 dB L/R.
- Relative to 1 kHz, 5 kHz approximately -4.94 dB, 10 kHz -23.06 dB, both
  channels. These agree closely with the other candidate modes.
- Reference RMS -59.44848 dBFS; output RMS -68.34537 / -68.34903 dBFS.
- Peaks -54.07792 / -54.08211 dBFS; no full-scale samples.

The first candidate capture set now supports dry/bypass agreement, intended
rates/stereo relationships for all three modes, and the proposed neutral wet
coloration at this test level. It does not complete musical listening, CPU,
physical transition, stereo-input or higher-level headroom validation.

Next requested physical state: Mode I/red, patch Mix noon, Tone/Width noon,
global Mix 100%, Input/Output unchanged. Confirm the actual LED because exiting
combined mode may return to a base mode selected before the hold action.
Next capture: `Endless vintageCandidate I mix50 tone50 width50 broadband peakMinus45 stereo 48k take1`.
After checking the blend, move to matched-level musical comparisons and
physical mode/control transition listening without assuming objective metrics
alone establish the final sound.
