# Mode II and I+II reference comparison, 2026-09-29

Eight new reference renders complete the three-mode technical capture set. All are nonzero stereo WAV files, exactly 74 seconds at 48 kHz. They use the same tone and broadband source items and gains as the Mode I captures. Plugin Mix stays fully wet, Roland/AIR added noise stays off, and factory output controls remain unchanged. No normalization, raw time shifting or pedal changes were made.

## Files and verified controls

Capture directory: `/Users/ryanveitch/Documents/REAPER Media/Polyend Endless/Chorus Capture 20260928/Plugin References/`.

Names: `<plugin> ModeII wet100 sine-and-broadband 48k take1.wav` and `<plugin> ModeIplusII wet100 sine-and-broadband 48k take1.wav`, for Roland, AIR Jura, Arturia JUN-6 and TAL-Chorus-LX. These take1 files are valid; only the earlier ModeI wet100 take1 files are invalid/silent.

`analyzeOtherModes.py` is the finite NumPy/SciPy analysis. `modeII-and-combined-comparison.json` contains exact paths, SHA-256 hashes, levels, spectral band ratios and both delay trajectories. Plugin versions remain those in the Mode I report. Hardware inputs are existing globalWet100/primaryWet100 recordings and their simultaneous ADAT3 references, with the same passthrough baseline correction.

| Plugin | Mode II parameter setting | I+II parameter setting | Verification |
|---|---|---|---|
| Roland | CHORUS TYPE param 1 = 0.5 | param 1 = 1 | Corresponding II / I+II lamps observed in native UI |
| Arturia | Chorus Mode param 2 = 1/3 | param 2 = 2/3 | REAPER formatted values read `Mode 2` / `Mode 1 + 2` |
| AIR | Chorus I param 0 = 0; Chorus II param 1 = 1 | both = 1 | Both parameters independently confirmed at 1 after restoring combined state |
| TAL | Chorus1 param 3 = 0; Chorus2 param 4 = 1 | both = 1 | Both parameters independently confirmed at 1 after restoring combined state |

The existing TwelveTake bridge exposes formatted parameter readback but its MCP wrapper does not; a bounded request used that existing bridge API. No bridge software was modified. Every plugin was returned to Mode I/full wet afterward. All track master sends are disabled, the transport is stopped, and the project is saved.

## Method and limits

Short-window broadband correlation uses 512 samples and a 20 ms step, versus 1024 samples/50 ms for the earlier Mode I report. Search range is 0-1800 samples, with parabolic peak refinement. Fitted sine rate search covers 0.1-12 Hz, enough to include the fast combined modes; the initial 9 Hz ceiling was explicitly widened after spectral peaks exposed boundary fits. Numbers below use the corrected wider search.

Delays are effective broadband correlation delays including filter phase. Roland subtracts the one-sample dry delay; Endless subtracts the 415-sample passthrough delay. They are not pure BBD-clock delay measurements. Motion within a window smooths extrema, particularly in the fast modes. A sine-fit rate is not a complete model of non-sinusoidal or multi-rate motion. No mode-specific dry-path retest was performed, so the previous Mode I dry corrections are assumed unchanged. Treat absolute offsets accordingly.

## Mode II

| Effect | Approx. rate Hz | Left effective delay range ms | Stereo audio correlation |
|---|---:|---:|---:|
| Roland | 0.819 | 1.60 to 5.57 | 0.0047 |
| AIR Jura | 0.862 | 1.71 to 5.36 | 0.0097 |
| Arturia JUN-6 | 0.748 | 1.22 to 5.60 | 0.0093 |
| TAL-Chorus-LX | 0.830 | 1.42 to 5.59 | 0.0045 |
| Endless | 0.862 | 1.90 to 4.77 | 0.0127 |

All five show opposing left/right delay trajectories. Endless rate closely matches AIR, but its roughly 2.87 ms sweep is shallower than the roughly 3.65-4.38 ms plugin ranges. This repeats the depth mismatch observed in Mode I, though the gap is smaller in Mode II.

## I+II / both enabled

| Effect | Dominant fitted rate Hz | Left effective delay range ms | Stereo audio correlation |
|---|---:|---:|---:|
| Roland | 9.200 | 3.39 to 3.66 | 1.0000 |
| AIR Jura | 9.750 | 0.87 to 1.54 | 0.0590 |
| Arturia JUN-6 | 8.018 | 3.14 to 3.59 | 1.0000 |
| TAL-Chorus-LX | 0.400 | 1.32 to 5.59 | 0.0144 |
| Endless | 0.512 | 2.25 to 4.69 | 0.0254 |

- Roland: fast and shallow, identical rendered L/R samples. Effective range is about 0.28 ms.
- Arturia: fast and shallow, essentially mono. Full-render L-R residual RMS is approximately 0.0039% of L RMS; range about 0.45 ms.
- AIR: fast and shallow but stereo, with opposing trajectories. Its shorter effective delay center differs substantially from Roland/Arturia. Range about 0.67 ms.
- TAL: both parameters read enabled, yet this configuration retains a slow, wide trajectory dominated by approximately 0.4 Hz. Do not call this a fast hardware-style combined mode or infer its internal implementation from these settings alone.
- Endless: slow, stereo, multi-rate blend. Delay spectrum contains approximately 0.513 and 0.863 Hz components; the table's 0.512 Hz fit captures only the dominant component and leaves about 0.293 ms RMS residual. Range about 2.43 ms.

The large spread among plugins makes averaging their combined modes inappropriate. Roland and Arturia agree qualitatively with the previously documented Juno-60 fast, shallow, substantially common modulation. This is corroboration of the existing research, not a new measurement of original hardware.

## Spectral comparison

Left wet band power below is relative to each effect's own 1 kHz band. Hardware ratios subtract the passthrough/reference band ratios first. Welch windows are 16384 samples; bands are one-third octave. These are time-averaged band-power comparisons, not static coherent transfer functions. Strongly attenuated upper bands can approach quantization/noise limits.

| Mode | Effect | 5 kHz relative dB | 10 kHz relative dB | 18 kHz relative dB |
|---|---|---:|---:|---:|
| ModeII | Roland | -5.00 | -17.93 | -65.65 |
| ModeII | AIR Jura | -3.62 | -11.09 | -21.21 |
| ModeII | Arturia JUN-6 | -6.91 | -24.56 | -78.69 |
| ModeII | TAL-Chorus-LX | -3.81 | -7.89 | -11.08 |
| ModeII | Endless | -1.98 | -6.10 | -12.20 |
| ModeIplusII | Roland | -4.98 | -17.90 | -65.64 |
| ModeIplusII | AIR Jura | -3.62 | -11.09 | -21.24 |
| ModeIplusII | Arturia JUN-6 | -6.84 | -24.43 | -87.88 |
| ModeIplusII | TAL-Chorus-LX | -3.85 | -7.91 | -11.11 |
| ModeIplusII | Endless | -1.97 | -6.10 | -12.21 |

## Recommended next milestone

Prepare a measured-model revision proposal before changing DSP:

1. Replace the current slow dual-rate I+II blend with a fast, shallow, common modulation candidate grounded in the existing hardware research. Preserve the current binary/branch for comparison. Plugin disagreement means exact fast rate and center should be selected from source evidence, not averaged.
2. Revisit the I/II depth mapping; both are shallow against this reference set. Keep a shared phase with opposing normal-mode trajectories and compare extracted curves rather than just LFO labels.
3. Investigate the neutral wet filter and gain independently. Endless is brighter than the Roland/Arturia examples. Retain raw-level evidence and avoid hiding gain differences with normalization.
4. Build a musical listening set and obtain Ryan's preference before final voicing. Keep the agreed simple final controls, rather than promoting the temporary diagnostic Mix range into the final product.

No DSP or pedal settings changed during this milestone. No authenticity winner has been selected.
