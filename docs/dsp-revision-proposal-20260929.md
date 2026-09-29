# DSP revision proposal: Juno-60 motion and vintage wet coloration

2026-09-29. Status: proposed candidate, not implemented or hardware-validated.

## Intent

Ryan explicitly wants a similar vintage tonal/EQ rolloff to the original units. The neutral setting must include that coloration, with a relatively static Juno-style presentation. Preserve the dry path, simple controls, current saved builds and measurement provenance. Do not substitute arbitrary hiss or saturation for the tonal response. Do not adjust the pedal's global Input/Output controls to make the DSP appear correct.

This proposal updates the current effect in place, without changing the SDK ABI or introducing a new DSP framework. The temporary diagnostic Mix mapping remains available during validation; final-product Mix should return to the previously requested broad fixed-blend region and fully-wet endpoint. The exact relationship between that final plateau and the requested 0-5% diagnostic dry zone must be resolved at control finalization, rather than silently changing it here.

## Evidence and choices

Read the [Mode I comparison](modeI-reference-comparison-20260929.md), [other modes](other-modes-reference-comparison-20260929.md), and [circuit evidence](research/juno-chorus-evidence.md). Source was re-read in full at this milestone. The current source's introductory claim that its slow dual-rate blend is more Juno-like is contradicted by the existing research and must be corrected with the implementation.

Three approaches were considered:

1. **Recommended: measured delay model with separately calibrated wet filtering.** Fix timing, modulation topology and coloration first. Retain straightforward fractional delay until a more detailed BBD model demonstrates useful improvement.
2. Match a single commercial plugin. Useful for controlled listening, but the plugins disagree, especially I+II. No plugin has been selected as the sole target.
3. Full circuit/clock simulation immediately. Potentially more accurate, but it adds complexity before we have established the simpler model's residual error or target CPU budget.

The proposal uses hardware-recording-derived motion anchors and a provisional tonal envelope informed by current plugin captures. This distinction matters: the tonal envelope below is a listening/calibration target, not a measured original-Juno specification.

## Motion candidate

| Mode | Rate | Nominal delay range | Stereo relationship |
|---|---:|---:|---|
| I | 0.513 Hz | 1.66-5.35 ms | Shared phase, opposing trajectories |
| II | 0.863 Hz | 1.66-5.35 ms | Shared phase, opposing trajectories |
| I+II | 9.75 Hz | 3.3-3.7 ms | Common approximately sinusoidal trajectory |

These are a coherent initial dataset from [Andy Harman's recording analysis](https://github.com/pendragon-andyh/Juno60/blob/beab8655781db7dadf2bf9fa6e3d4b6514de3132/Chorus/README.md), refreshed on this date. They are not factory tolerances. Later asymmetric range refinements remain documented for a subsequent fit. Do not combine extrema from different reference units without provenance.

For the first candidate use triangle delay trajectories in I/II and sine in I+II. Label this a delay-domain approximation; linear triangle control voltage is not proof that a real clock/BBD produces a perfectly linear triangle delay. Keep phase advancing continuously, and implement a short transition crossfade between old and new wet renders rather than jumping read positions. Start with 20 ms as an engineering candidate, then test clicks and listening; it is not an analog-circuit constant.

Correct the known one-sample delay indexing error before calibrating the range. Separate nominal delay from filter group delay in tests. Keep the existing experimental build as the comparison baseline.

## Wet coloration is a first-class requirement

Signal flow: original stereo dry bypasses wet processing; documented mono wet excitation feeds an input conditioning filter, two modulated delay reads, matched reconstruction filters, then explicit wet gain and mixing. Do not move all filtering onto the mixed output: that would dull the preserved dry path. Keep the existing stereo wet-input summing policy explicit, including cancellation of anti-phase wet excitation; do not infer wiring from silence.

At neutral Tone, adopt these provisional wet-only band targets relative to the wet 1 kHz band:

| Frequency | Initial calibration target | Why |
|---|---:|---|
| 100 Hz | Within approximately 1 dB of 1 kHz | Avoid inventing a large bass cut without circuit evidence |
| 5 kHz | -4 to -7 dB | Includes the darker reference voicings |
| 10 kHz | -18 to -25 dB | Meaningful vintage high-frequency attenuation |
| 18 kHz | Report, do not tightly fit yet | Deep attenuation may approach render/noise limits |

Measured current Endless values are approximately -1.96 dB at 5 kHz and -6.09 dB at 10 kHz, relative to 1 kHz. The proposed change is material, not a cosmetic adjustment to the current 8.5 kHz one-pole control.

A concrete host prototype starts with one second-order low-pass before the delay at 8 kHz and one after each delay at 4.5 kHz, Q = 1/sqrt(2), designed for 48 kHz. An independent SciPy digital-response calculation predicts approximately -4.58 dB at 5 kHz, -22.37 dB at 10 kHz and -60.89 dB at 18 kHz, relative to 1 kHz, for the filter cascade alone. This is a deliberately simple fit candidate, NOT a transcription of the Juno schematic. Delay interpolation and modulation change the final response, so coefficients must be adjusted against full-path renders. Do not quote these predicted values as measured pedal output.

Preserve a 40-60% neutral Tone plateau, now meaning the calibrated vintage response. Outside it, allow modest darker/brighter variation while retaining the input conditioning stage. The bright end must not silently bypass all vintage filtering. Start with a +/-20% reconstruction-cutoff variation, test it, and retain fixed neutral coefficients as the reference.

The BBD modeling paper supports treating input and reconstruction filtering explicitly, but its example components are not Juno calibration values: [Raffel and Smith, DAFx 2010](https://colinraffel.com/publications/dafx2010practical.pdf). The manufacturer [chorus-board drawing](https://www.florian-anwander.de/roland_string_choruses/JUNO-60_schem_chorus.jpg) remains the source for any later claimed circuit-derived filter. Exact component-derived transfer and loading have not been solved in this proposal.

## Gain, nonlinear behavior and width

For calibration, replace the unexplained tanh drive/makeup stage with explicit linear wet gain and disable added noise. Begin at unity low-frequency wet gain, measure the resulting blend, and set a documented level target from listening and raw measurements. Unity is a diagnostic starting point, not an assertion about analog wet gain. Preserve prior tanh/noise behavior in the saved baseline, not as an assumed vintage requirement.

Introduce nonlinear character later only if level-dependent reference tests and listening support it. Do not add a compander without Juno-specific evidence. Never fit quiet-test distortion to the unrelated pedal-path harmonic measurements.

Neutral Width preserves opposing normal-mode delays; I+II remains common modulation. Avoid the current extra side gain above unity until headroom tests justify it. First-candidate width may narrow wet stereo from mono to nominal, with the upper range clamped at nominal. This prevents an undocumented stereo enhancement from being mistaken for original behavior.

## Implementation boundaries and verification

Primary changes belong in `example/source/PatchImpl.cpp`, with small directly testable filter/delay helpers only as needed. Keep no-heap audio processing, 48 kHz, SDK entry points and footswitch/LED assignments. Compute filter coefficients on parameter changes, not per sample. Retain raw left/right samples unchanged in the 0-5% diagnostic dry zone.

Required focused checks:

- Integer and fractional impulse delays, including buffer wrap, then verify nominal modulation extrema and rates independently of filtering.
- Exact dry samples, including signed zero and anti-phase stereo; no wet filter/noise leakage into dry.
- Fixed-delay magnitude and phase, then modulated broadband band power. Compare both to the declared targets, not a static transfer estimate applied to a time-varying system.
- I+II common wet output for identical mono excitation; I/II opposing delay motion. Test rapid mode changes and parameter ramps.
- Finite output, silence behavior and headroom across tone/width/mix extremes at realistic and near-full-scale inputs. Fix gain budgets instead of hiding overshoot behind an arbitrary clipper.
- Fresh warning-clean ARM build, finite host tests, and hardware capture with the existing loopback reference. Actual pedal CPU and listening validation remain required.

Keep global Mix fully wet for diagnostic isolation and leave global Input/Output unchanged. After loading a new binary, sweep the primary knobs before captures. Do not request new physical tests until the candidate exists.

Planned measurement names, once a build is ready:

- `Endless vintageCandidate I dry0 tone50 width50 stereo 48k take1`
- `Endless vintageCandidate I wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`
- `Endless vintageCandidate II wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`
- `Endless vintageCandidate IplusII wet100 tone50 width50 broadband peakMinus45 stereo 48k take1`

Use take2 for repeats and separately named simultaneous ADAT3 reference files. The dry0 capture uses the same broadband stimulus and level. Record exact build hash and parameter state with every take.

## Acceptance and next step

Implement the candidate only after this proposal is accepted. First compare objective modulation and wet coloration, then prepare a common musical excerpt at the ordinary blend for listening. Preserve raw-level comparisons and supply separately labeled loudness-matched audition copies where useful. Ryan's listening acceptance determines final voicing; passing a plugin comparison alone does not establish original-hardware equivalence or a finished product.
