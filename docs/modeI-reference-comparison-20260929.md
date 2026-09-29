# Mode I dry-path and wet comparison, 2026-09-29

## Dry controls

Four additional 74-second renders set only each plugin's Mix to zero. Mode I, noise controls, output controls and source gains were retained. Mix was restored to fully wet after each render. Session saved with every track master send disabled, transport stopped. No pedal controls changed.

Files: `Plugin References/<plugin> ModeI dry0 sine-and-broadband 48k take1.wav` under the existing capture directory. See `modeI-comparison.json` for hashes. AIR, Arturia and TAL reproduce the original broadband samples exactly in both channels over seconds 43-71, with zero sample offset. Roland's dry output has a one-sample delay (0.020833 ms), -0.00265 dB gain and 0.00316% residual RMS after delay/gain fitting. Its measured band gains are flat within 0.00004 dB at the sampled frequencies. This is consistent with essentially unity dry transfer plus one sample of delay.

These are residual delays in offline rendered audio under REAPER's compensation behavior, not plugin-reported latency, interface round-trip latency or a live monitoring measurement.

## Wet comparison

`analyzeModeI.py` and `modeI-comparison.json` are saved beside the audio. The finite Python analysis uses NumPy/SciPy. It reads existing valid wet take2 files and current dry take1 files. Hardware files are the previously captured Mode I globalWet100/primaryWet100 noise recording and its simultaneous ADAT3 reference, plus passthrough take1 and reference. Raw files are untouched.

Delay estimates use 1024-sample broadband windows every 50 ms, a 0-1800 sample correlation search, absolute peak selection and parabolic refinement. A known 173-sample synthetic delay passed the estimator sign/index check. Values below are effective correlation delays, including filter phase effects, not isolated BBD clock delay. They are preliminary estimates from one capture per plugin, not confidence intervals. Wet median correlation is approximately 0.51-0.53 for plugins and 0.70 for Endless, reflecting filtering and time variation within each window. Finite windows smooth extrema.

Endless delays subtract the previously measured 415-sample passthrough delay relative to ADAT3. Roland values in this table subtract its measured one-sample dry delay. Other plugins need no dry correction. Absolute delay comparisons retain analog/filter uncertainty. Rates are sine fits to the measured trajectories; non-sinusoidal motion remains in the residuals.

| Effect | Approx. rate Hz | Left effective delay range ms | Right effective delay range ms |
|---|---:|---:|---:|
| Roland | 0.479 | 1.60 to 5.56 | 1.60 to 5.56 |
| AIR Jura | 0.513 | 1.71 to 5.36 | 1.69 to 5.35 |
| Arturia JUN-6 | 0.409 | 1.23 to 5.60 | 1.22 to 5.60 |
| TAL-Chorus-LX | 0.400 | 1.33 to 5.19 | 1.32 to 5.19 |
| Endless ModeI | 0.513 | 2.75 to 4.61 | 2.76 to 4.62 |

The current Endless sweep spans about 1.87 ms, versus approximately 3.65-4.38 ms across these references. This supports investigating modulation depth, while preserving user listening judgment. Its rate closely matches AIR; agreement on rate alone does not establish matching modulation shape.

## Wet coloration

Welch power spectra use 16384-sample windows; reported values integrate one-third-octave bands. This is an output/input band-power comparison for a time-varying effect, not a static coherent transfer function. Hardware ratios are corrected by the matched passthrough/reference power ratios. The following left-channel values are relative to each effect's own 1 kHz band to separate broad tonal shape from wet gain. Raw unnormalized gains remain in JSON.

| Effect | 5 kHz relative dB | 10 kHz relative dB | 18 kHz relative dB |
|---|---:|---:|---:|
| Roland | -4.99 | -17.92 | -65.70 |
| AIR Jura | -3.61 | -11.07 | -21.19 |
| Arturia JUN-6 | -6.87 | -24.53 | -78.63 |
| TAL-Chorus-LX | -3.81 | -7.89 | -11.09 |
| Endless ModeI | -1.96 | -6.09 | -12.20 |

Endless is brighter than Roland and Arturia at these settings, and closer to TAL in the upper bands. The references themselves vary substantially. Deep attenuation values near the render/noise floor should not be read as precise filter specifications. No reference is established as original-hardware truth and no DSP changes are justified solely by brand.

## Next steps

1. Capture and analyze Mode II and I+II with the same dry/noise/output conventions. Verify enum mappings before changing them.
2. Use the three-mode comparison plus circuit research to propose changes to modulation depth, I+II behavior and wet filtering, with clear distinction between measured emulator behavior and original Juno evidence.
3. Make a musical A/B listening set using ordinary mix settings before selecting final voicing. Keep the pedal's global Input/Output unchanged.
