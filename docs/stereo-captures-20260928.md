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
