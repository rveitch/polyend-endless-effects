# Juno-60 chorus evidence

Research checked 2026-09-19. This separates manufacturer circuit evidence, analysis of recordings, and choices made by software authors. No hardware measurements were performed for this project.

## Findings and confidence

| Claim | Finding | Confidence |
| --- | --- | --- |
| Shared triangle modulation | One LFO feeds two control paths; the normal modes invert one path. The combined mode changes that behavior. | High for circuit topology |
| Two delay chips | The chorus-board drawing identifies IC2/IC4 as MN3009 and IC1/IC3 as MN3101. | High, manufacturer schematic |
| I and II rates | 0.513 and 0.863 Hz are recording-derived estimates. The service drawing shows approximately 0.5 and 0.83 Hz. | High attribution; medium as universal calibration |
| Delay range | 1.66 to 5.35 ms is an early recording estimate. Later analysis finds asymmetric channel ranges. | Medium, reference-unit dependent |
| Both buttons | Fast, shallow, substantially common modulation; not independent slow/fast choruses layered together. | High across schematic and recording research |
| Compander | No chorus compressor/expander pair is identified in the Juno-60 drawing. Adding one is not justified as a fidelity requirement. | High practical conclusion |
| Exact filter model | Filtering is clearly present, but a simple 12 dB/octave statement is not a verified description of the whole wet path. | Full transfer function remains to be derived |

## Manufacturer schematic

The [Roland service notes scan](https://medias.audiofanzine.com/files/roland-juno-60-service-notes-472222.pdf) is a manufacturer document hosted by a third party. Directly inspected the [chorus-board drawing, printed page 11](https://www.florian-anwander.de/roland_string_choruses/JUNO-60_schem_chorus.jpg) and [panel chorus-LFO drawing](https://www.florian-anwander.de/roland_string_choruses/JUNO-60_schem_LFO.jpg).

The chorus board has two MN3009/MN3101 pairs, bias trims, filtering around the wet path, and separate direct/wet summing at IC8. There is no identified envelope-controlled compander pair. The LFO drawing shows triangle periods of 2 seconds and 1.2 seconds for I and II, with 20 Vpp labels. Its combined-mode sketch is smaller and rounded, labeled 1.6 Vpp. The printed 1-second annotation conflicts with measured fast-mode behavior; treat a missing zero as a plausible typo, not an established fact.

The LFO drawing also records a factory modification from serial 26550 to reduce modulation amplitude and clock leakage. This is concrete evidence that revision and unit variation matter when interpreting exact measured values.

## Recording analysis provenance

[Andy Harman's chorus notes](https://github.com/pendragon-andyh/Juno60/blob/beab8655781db7dadf2bf9fa6e3d4b6514de3132/Chorus/README.md#L5-L39), pinned commit `beab8655781db7dadf2bf9fa6e3d4b6514de3132`, describe visual analysis of recordings in Sonic Visualizer. The original summary gives I/II at 0.513/0.863 Hz with 1.66 to 5.35 ms delay, and I+II at 9.75 Hz with 3.3 to 3.7 ms delay and mono behavior. These are estimates from reference recordings, not factory tolerances. The author explicitly cannot identify companding in the schematic. The 12 dB filter statement is attributed to a forum discussion, not a circuit derivation. Likewise, the suggestion of BBD bit crushing should not be adopted: a BBD samples time but does not quantize amplitude into digital bits.

[Cimalando's later measurement](https://github.com/jpcima/rc-effect-playground/issues/2#issuecomment-541576525) refines one channel to 1.54 to 5.15 ms and reports triangle duty about 49.3333%. [The combined-mode fit](https://github.com/jpcima/rc-effect-playground/issues/2#issuecomment-541340615) is approximately sinusoidal at 9.75 Hz, 3.22 to 3.56 ms. Earlier comments used a mixed-down YouTube signal and produced wrong estimates; [the researcher explicitly discarded that reference](https://github.com/jpcima/rc-effect-playground/issues/2#issuecomment-541194768). Do not cite the early 0.95/1.6 Hz guesses as independent corroboration.

The [mixing analysis](https://github.com/jpcima/rc-effect-playground/issues/2#issuecomment-542337202) identifies 100k feedback, 47k direct and 39k wet summing resistors. Their relative direct/wet gain is 39/47, about 0.83 or -1.62 dB. This is the summing-stage ratio, not a full calibration of the two complete paths.

## DSP research

[Raffel and Smith, DAFx 2010](https://colinraffel.com/publications/dafx2010practical.pdf), especially sections 2 and 4, supports modeling filters plus variable-delay behavior, with clock-driven sampling for greater fidelity. Section 2 gives constant-clock delay `N / (2 fcp)`, where `fcp` is the two-phase clock frequency. With N=256 and 1.66 to 5.35 ms, that implies roughly 77.1 to 23.9 kHz, not a fixed 70 kHz. Section 1.3 explicitly distinguishes short chorus/flanger circuits, which often omit companders, from longer echoes. The paper's example component values and nonlinear measurements are not Juno-60 calibration data. [Its author provides example code and filter-generation resources](https://colinraffel.com/software/bbdmodeling/).

[Russell McClellan's BBD LFO application note](https://www.russellmcc.com/conformal/app_notes/2-bbd-lfo/) explains a further subtlety: instantaneous `N / clockRate(t)` ignores clock history during transit. With an edge-count clock convention, exact delay is `t - C^-1(C(t)-N)` where `C` integrates clock rate. A proposed inexpensive approximation smooths the delay trajectory using `y[n] = alpha*x[n] + (1-alpha)*y[n-1]`, with `alpha = 1-exp(-2/averageDelaySamples)`. This is useful algorithm research, not a measured Juno circuit model. Distinguish its edge-count convention from the DAFx two-phase frequency convention before importing formulas.

## Engineering consequences and remaining unknowns

Start normal modes with a shared modulation phase, opposing delay trajectories, short delays, and a filtered wet path. Model authentic both-buttons behavior separately from an optional parallel-chorus mode. A simple delay model can be a useful baseline, but should not be labeled a verified circuit emulation.

Still unverified: actual target-unit delay curves, full filter magnitude/phase response, level-dependent distortion, noise spectrum and amplitude, clock leakage, and CPU cost of a clock-driven model on the target device. Do not assume the Juno-106 has identical rate, depth, gain, and filtering merely because it shares the BBD family. This research establishes Juno-60 parameters; a 106 mode needs separate schematic and recording evidence.
