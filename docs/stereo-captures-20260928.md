# Stereo pedal captures, 2026-09-28

## Scope and provenance

Characterization of the current diagnostic chorus build, not verification against a Juno reference. User confirmed pedal modes and knob settings. Two captures each of I, II and I+II, Mix 100%, Tone 50%, Width 50%. No DSP changes made during these recordings.

Loaded build reported in this session: `juno_chorus_mix_test_20260928_152517.endl`, SHA256 `a2394ba05edd26067456d7a50a0c48fabeac5d844a1b6c62e41cf64678804268`. The pedal binary cannot be read back by this capture workflow.

Session: `/Users/ryanveitch/Documents/REAPER Media/Polyend Endless/Chorus Capture 20260928/Endless Stereo Capture.RPP`.
Raw media, `capture-manifest.json`, `bypass-analysis.json`, and `wet-modes-analysis.json` are alongside the session. Manifest includes individual WAV hashes. Raw audio is not stored in Git.

## Setup

- REAPER 7.80, SSL 18 + Alpha 8, 48 kHz, 24-bit recorded WAV, 128-sample device buffer.
- Mono output ADAT 1 to mono Endless input via TS. Stereo insert cable returns Endless L/R to ADAT 1/2.
- Simultaneous balanced direct loopback ADAT 3 output/input supplies reference.
- 34-second stimulus: 2 seconds silence, 30 seconds 1 kHz sine with 20 ms end fades, 2 seconds silence.
- Source RMS -24 dBFS; stimulus track gain -24 dB for the wet captures gives nominal -48 dBFS RMS. Reference measured approximately -48.175 dBFS.
- No FX or master sends on capture tracks; return monitoring off. Driver-reported recording latency compensation enabled, manual offsets zero.
- Time selection auto punch gives 34-second items. Raw WAVs can extend beyond the punch window. Prompt on stop disabled. Automation uses native Record action 1013 because the connector's dedicated Record command failed.
- Steady-state analysis covers raw WAV seconds 3 through 31. No normalization or alignment applied to original media. Shared reference is retained; script start/stop time is not a latency measurement.

## Observations

| Mode | Take 1 RMS L / R, dBFS | Take 2 RMS L / R, dBFS | L/R correlation, takes 1 / 2 |
| --- | --- | --- | --- |
| I | -48.829 / -48.794 | -48.802 / -48.851 | 0.331 / 0.328 |
| II | -48.964 / -48.976 | -48.982 / -48.966 | 0.317 / 0.316 |
| I+II | -48.863 / -48.878 | -48.893 / -48.914 | 0.334 / 0.399 |

No full-scale samples in any wet or reference take. Both channels and all references are present. Reference RMS changed less than 0.001 dB across these takes.

Complex demodulation at 1 kHz (30 Hz fourth-order Butterworth low-pass, forward/backward filtering) followed by phase unwrapping gives repeating phase peaks near 0.513 Hz for I and 0.864 Hz for II. Both I+II takes contain prominent phase spectrum components near both rates (FFT bins 0.500 and 0.857 Hz, 28-second observation). Do not reduce combined mode to a single inferred LFO rate. Width mixing means output phase excursion does not directly measure a single delay tap. Absolute delay remains ambiguous by whole tone periods.

I+II is distinctly stereo in this test. Correlation depends on stimulus, window and control settings and is not a broadband stereo-width rating. These measurements characterize the existing dual-rate implementation; they do not establish fidelity to original Juno I+II behavior.

## Bypass level concern

Before wet captures, nominal bypass stimulus levels -24, -36 and -48 dBFS RMS produced approximately 7.6%, 0.35% and 0.021% harmonics respectively. This is the root-sum-square of harmonics 2 through 20 divided by the fundamental over a 28-second steady segment, not THD+N. Direct reference was substantially cleaner. The level dependence suggests nonlinearity somewhere in the pedal path, but its analog/DSP location is not established. Digital peak levels alone cannot exclude analog overload. Earlier REW results do not explain this discrepancy yet.

Use -48 dBFS for like-for-like comparisons until routing, converter operating levels, pedal input gain and bypass state are independently checked. Do not silently recalibrate this away.

## Next work

1. Resolve the bypass level-dependent distortion before final gain/headroom decisions.
2. Use a suitable broadband or multi-frequency stimulus and documented reference captures to estimate wet response and delay behavior. This single tone cannot establish absolute wet delay or overall tonal accuracy.
3. Compare the intended Juno I+II target with the current stereo dual-rate behavior before changing DSP.
4. Preserve the diagnostic Mix mapping until validation is complete, then restore the agreed mostly static final-product control behavior.

## Direct TS follow-up

User removed the pedal and connected Alpha 8 output 1 directly to input 1 using the existing TS send cable; ADAT 3 reference and interface settings were unchanged. Recording track switched to mono input ADAT 1.

| Nominal stimulus RMS | Measured direct RMS | Harmonics 2-20 / fundamental |
| --- | --- | --- |
| -24 dBFS | -24.3543 dBFS | 0.000219% |
| -48 dBFS | -48.3544 dBFS | 0.001248% |

The measured level change was 24.0002 dB. Neither direct take clipped digitally. Reference RMS remained consistent with preceding captures. See `direct-TS-analysis.json` and the updated manifest beside the REAPER session for raw filenames and hashes.

This excludes the direct channel-1 interface/cable path as the source of the previously measured large harmonics under this configuration. It does not distinguish pedal input electronics, firmware bypass processing, return connection, or an incorrectly identified earlier pedal state. Next: reconnect the pedal, explicitly establish bypass, and repeat the -24 dBFS capture before attributing the distortion to a particular stage.

## Explicit bypass recheck after reconnection

User restored the pedal cabling and explicitly confirmed bypass. At nominal -24 dBFS RMS, measured L/R RMS was -25.0536/-25.0685 dBFS, with harmonic ratios 7.66498%/7.54250%. The simultaneous reference remained -24.1749 dBFS with 0.0001845% harmonics. This closely reproduces the initial bypass capture after reconnection. No full-scale samples. `bypass-recheck-analysis.json` beside the REAPER session records file hashes.

Next diagnostic: engage the current chorus with primary Mix swept to zero, leaving firmware secondary Input/Output/Mix untouched, and compare the same stimulus. This tests whether the observed distortion also occurs through the patch's validated dry path, without conflating it with wet-path saturation. Firmware secondary control scaling and exact signal ordering remain undocumented.

## Engaged dry recheck and correction to REW interpretation

With the pedal engaged and primary Mix swept to zero, nominal -24 dBFS tone measured -25.0516/-25.0683 dBFS RMS L/R and 7.66511%/7.54271% harmonic ratios. This matches bypass closely; the wet chorus algorithm is not required for the observed distortion. `dry0-recheck-analysis.json` contains raw filenames and hashes.

Retrospective REW API distortion inspection of the four saved `Endless mixTest I bypass R` and `dry0 R` takes found **3.01% THD at 1000 Hz in all four**, dominated by H3. Earlier gain/timing comparisons had not inspected this data. Thus distortion was already present in those REW measurements; a matching bypass/dry response did not establish a distortion-free path. `rew-prior-distortion.json` preserves UUIDs and exported 1 kHz rows. REW sweep and REAPER steady-tone values use different methods and possibly effective levels; their numerical difference is not yet explained.

## Power-cycle persistence check

User powered off, restarted holding only the left switch to select mono input, engaged the effect, and swept primary Mix to zero. At nominal -24 dBFS RMS, measured L/R RMS was -25.0508/-25.0679 dBFS and harmonic ratios 7.66245%/7.54021%, effectively unchanged from the preceding dry test. Reference remained -24.1748 dBFS. Results and hashes are in `afterRestart-analysis.json`. This restart did not remove the distortion; it does not prove whether firmware secondary values were retained or reset to equally distorting defaults.

## Clear-effect reset and passthrough reload

User performed the both-switch power-on clear-effect procedure, loaded the prior passthrough diagnostic and confirmed bypass, then engaged it for a second capture. The test was at the same nominal -24 dBFS RMS stimulus, with no requested secondary-control adjustment.

| State | RMS L/R, dBFS | Harmonics L/R, percent |
| --- | --- | --- |
| Bypass after reset | -25.05075 / -25.06786 | 7.66165 / 7.53927 |
| Engaged passthrough after reset | -25.05075 / -25.06789 | 7.66181 / 7.53941 |

The simultaneous direct reference remained clean and stable. Raw filenames and hashes are preserved in the two `passthrough-afterReset-*-analysis.json` files and capture manifest beside the REAPER session. The reset/reload did not remove the distortion; bypass and no-op patch processing are effectively identical at this level. This substantially isolates the issue from chorus DSP but does not locate it within the shared firmware, analog stages or return connection. The procedure is not evidence that secondary gains were reset. Next proposed variable: explicitly set secondary Input to 25 percent while retaining the passthrough, Output and global Mix settings.

## Passthrough send-level series and scope decision

Ryan reports no audible distortion and prefers leaving pedal secondary Input/Output levels unchanged, reflecting normal usage. The proposed Input25 experiment was not performed. Only REAPER stimulus gain changed for this series, with passthrough engaged.

| Nominal RMS send | Return RMS L/R, dBFS | Harmonics 2-20 L/R, percent |
| --- | --- | --- |
| -30 dBFS | -29.7790 / -29.8105 | 1.4574 / 1.4521 |
| -36 dBFS | -35.5003 / -35.5327 | 0.35496 / 0.35393 |
| -42 dBFS | -41.4315 / -41.4640 | 0.08805 / 0.08782 |
| -48 dBFS | -47.4144 / -47.4470 | 0.02150 / 0.02141 |

All eight pedal/reference WAVs were checked; no full-scale samples. References tracked the nominal send approximately 0.175 dB lower. Raw file hashes and metrics are in `passthrough-level-series-analysis.json`. These metrics are tone harmonic ratios, not audibility judgments or a declaration that the pedal is defective. Keep -48 dBFS RMS sine stimulus for controlled like-for-like comparisons with existing mode captures. This is not a prescribed musical operating level or evidence that the source of nonlinearity is fixed.

Next characterization step: obtain a deterministic broadband passthrough/reference baseline, then use the identical stimulus with the chorus reloaded to examine frequency-dependent behavior. Match and document peak as well as RMS levels for that different stimulus; do not assume the same RMS alone gives equivalent input excursions.

## Broadband passthrough baseline

Two engaged passthrough takes captured with unchanged pedal settings. Deterministic Gaussian noise, seed 20260928, FFT band-limited to 20 Hz-20 kHz, 30 seconds with 20 ms fades and two seconds of silence each end. 48 kHz 24-bit PCM; peak -45.000 dBFS, steady RMS -59.256 dBFS, stimulus track at 0 dB. The different RMS is intentional to constrain peak excursion. `broadband-stimulus.json` stores generation details and SHA256.

Named `Endless passthrough broadband peakMinus45 stereo 48k take1` and `take2`, each with simultaneous ADAT3 reference. Session positions 1000 and 1050 seconds. Analysis and hashes: `broadband-passthrough-analysis.json`; full frequency/complex transfer/coherence arrays in the four `broadband-passthrough-take*-channel*.npz` files beside the session.

Cross-correlation gives 415 samples, or 8.6458 ms at integer-sample resolution, for both channels and takes relative to the direct reference. This agrees with earlier REW relative delay near 8.6509 ms within sample resolution; neither is absolute interface round-trip latency. Original recordings remain unshifted. Analysis only aligns by the measured integer delay.

Welch H1 transfer used 65536-sample Hann windows, 50 percent overlap and raw seconds 3-31. Six 1/12-octave bands centered at 50, 100, 1000, 5000, 10000 and 18000 Hz repeat within 0.002 dB; all band coherences exceed 0.99999. At 1 kHz, L/R gains relative to ADAT3 are approximately +0.766/+0.732 dB. Different converter/cable paths are included; this is not isolated pedal gain. Raw return peaks are about -44.11/-44.14 dBFS.

Next: reload the same chorus mix-test binary, sweep primary knobs, set I / fully wet / Tone50 / Width50 and capture the identical broadband signal. A modulated chorus is time-varying: reduced coherence and H1 attenuation can reflect modulation, so do not interpret its long-window H1 as an ordinary static EQ response. Preserve time-varying evidence and compare output power spectra as well.

## Mode I broadband captures, 2026-09-29

User confirmed the same chorus mix-test build reloaded, engaged in I, primary Mix100/Tone50/Width50, secondary settings untouched. Two captures of the identical broadband stimulus at session positions 1100 and 1150 seconds. Results and raw file SHA256 hashes: `broadband-chorus-I-analysis.json` beside the session. Reference RMS -59.44855/-59.44858 dBFS matches the preceding-day baseline to within 0.001 dB. No full-scale samples in either stereo return.

Return RMS is approximately -62.36 dBFS on both channels; L/R correlation 0.5912 and 0.5916. For 1/3-octave integrated output/reference power relative to corresponding passthrough baseline, 1 kHz is about -1.2 to -1.3 dB, 5 kHz -2.6 dB, 10 kHz -4.4 dB and 16 kHz -5.4 dB. At 100 Hz results span -4.93 to -5.50 dB across channels/takes. These are stationary-noise power comparisons including modulation and added noise, not an isolated filter response or perceived loudness rating. No absolute wet delay inferred.

Next: identical two-take broadband captures for Mode II, changing only the mode; then I+II.

## Mode II broadband captures, 2026-09-29

User confirmed green Mode II engaged, knobs unchanged from the Mode I wet100/Tone50/Width50 test. Identical broadband signal captured twice at session positions 1200 and 1250. Full 34-second windows present on stereo returns and references; no full-scale return samples. See `broadband-chorus-II-analysis.json` beside the session for metrics, exact filenames and hashes.

Return RMS approximately -62.35 to -62.36 dBFS; reference -59.44863 dBFS, stable against the baseline. L/R correlation 0.58952/0.58964 versus Mode I 0.59124/0.59164. Third-octave output/reference power relative to passthrough is approximately -1.3 dB at 1 kHz, -2.6 at 5 kHz, -4.4 at 10 kHz and -5.4 at 16 kHz, close to Mode I. At 100 Hz the four channel/take results range -3.37 to -3.67 dB, versus Mode I -4.93 to -5.50 dB. These are modulated broadband power comparisons, not static EQ or audibility conclusions.

Next: I+II broadband takes at the same controls and stimulus.

## I+II broadband captures, 2026-09-29

User confirmed blue I+II engaged with controls unchanged. Identical broadband signal captured twice at positions 1300 and 1350 seconds; full 34-second windows present, no full-scale samples. Results and raw SHA256 hashes: `broadband-chorus-IplusII-analysis.json` beside the session. Reference RMS -59.44859 dBFS in both takes. Return RMS approximately -62.35 to -62.36 dBFS, L/R correlation 0.59634/0.59686.

Third-octave output/reference power relative to passthrough is approximately -4.4 to -4.6 dB at 100 Hz, -1.35 to -1.49 at 1 kHz, -2.6 at 5 kHz, -4.4 at 10 kHz and -5.4 at 16 kHz. All three modes show similar overall broadband RMS and upper-band power at these settings. I+II remains stereo, consistent with the current combined modulation implementation rather than proof of authentic Juno both-button behavior.

The fully wet broadband set is complete. Next proposed check: primary Mix at noon (nominal 50/50) in Mode I with Tone/Width unchanged, to characterize the intended dry/wet blend using the same stimulus. Do not change secondary controls.

## Primary noon capture and wet-label qualification, 2026-09-29

User set red Mode I and primary Mix to noon, Tone/Width unchanged. Two broadband takes at 1400/1450 seconds, no full-scale samples, reference stable near -59.44861 dBFS. Output RMS approximately -60.65/-60.68 dBFS L/R; about 1.98 dB below the reference-normalized passthrough. L/R correlation 0.94171/0.94147. Metrics and raw hashes: `broadband-chorus-I-mix50-analysis.json`.

A least-squares projection onto the identical passthrough waveform, aligned using simultaneous references (zero integer offset), estimates dry coefficients near 0.770 for both channels/takes. Retrospective projection of all six earlier broadband `wet100` takes gives 0.4983-0.4999, not approximately zero. `broadband-dry-projection-crosscheck.json` preserves these cross-check values. Wet-path correlation, sub-sample alignment and hardware differences can bias this estimate, but the repeated near-half dry coefficient is strong evidence against interpreting those recordings as isolated wet output.

**Correction:** `wet100` filenames record the requested primary knob position, not independently verified 100 percent wet system output. The physical noon captures also do not yet establish an exact 50/50 overall blend. A second firmware/global dry/wet blend is a leading hypothesis; actual global state has not been read back. No secondary Input or Output changes are proposed. Next proposed diagnostic: set only secondary/global Mix fully wet, restore primary Mix fully clockwise, and repeat one Mode I broadband capture to check whether the dry component disappears. Existing raw captures must be retained with their original names and this qualification.

## Global Mix full-wet diagnostic, 2026-09-29

User authorized setting only secondary/global Mix fully wet despite preferring default/normal-use settings. Primary Width restored to noon and primary Mix fully clockwise, Mode I engaged, Input/Output unchanged. One broadband capture at session position 1500. `broadband-globalWet100-primaryWet100-analysis.json` contains exact filenames, hashes and analysis details.

Dry projection coefficients fell from approximately 0.499 in the earlier primary wet100 takes to -0.00063/-0.00189 L/R, consistent with zero within the limitations of correlated-noise projection. L/R correlation fell from approximately 0.591 to 0.01665. Output RMS -60.1536/-60.1481 dBFS; stable reference -59.44864 dBFS; no full-scale samples. This controlled change strongly supports the global Mix layer as the source of the previous additional dry blend. Negative tiny projection values are estimation residuals, not negative physical dry gain.

Treat this as the first verified wet-only broadband Mode I capture. The previous `wet100` files are valid captures of requested primary positions under an unknown global blend, not isolated wet output. Preserve them as evidence of the user's previous overall sound. Factory global Mix default remains unknown, and clear-effect reset/reload has not been shown to restore global controls. If restoration is requested, compare measured blend against the saved earlier baseline rather than assume reset semantics. Next: wet-only broadband II and I+II without further global-control changes; then verify primary noon blend.

## Mode II with both mixes fully wet, 2026-09-29

User confirmed green Mode II; global Mix and primary Mix remain fully wet, Tone/Width noon, Input/Output unchanged. Capture position 1550, full 34-second stereo and reference windows. `broadband-II-globalWet100-primaryWet100-analysis.json` contains filenames, hashes and methods. Dry projection coefficients -0.00078/-0.00115 L/R are consistent with approximately zero; output RMS -60.1485/-60.1433 dBFS, L/R correlation 0.01274. Stable reference -59.44869 dBFS and no full-scale samples. These results support wet-only Mode II capture, with estimation caveats unchanged. Next: I+II with both mixes fully wet, then primary noon verification.

## I+II with both mixes fully wet, 2026-09-29

User's proceed response was interpreted as confirming the requested blue I+II state; knobs unchanged. Position 1600, full 34-second capture and reference. `broadband-IplusII-globalWet100-primaryWet100-analysis.json` stores exact filenames, hashes and method. Dry projection -0.00371/-0.00104 L/R, near zero within correlated-wet projection limitations; output RMS -60.1603/-60.1575 dBFS; L/R correlation 0.02544. Stable reference -59.44869 dBFS and no full-scale samples. Wet-only broadband characterization now includes all three modes. Next physical step: Mode I red, primary Mix noon while global Mix remains fully wet; verify actual dry contribution before treating physical noon as precisely 50/50.
