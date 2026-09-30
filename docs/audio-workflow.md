# Reproducible stereo captures

The host probe runs one selected effect, ends after the input and writes stereo IEEE float32 WAV. Default synthetic captures last 12 seconds at 48000 Hz. Each capture has an exact input WAV and JSON provenance sidecar. It preserves both channels, raw gain and raw timing. These files are host evidence; pedal captures still need the wiring/calibration record.

```sh
python3 scripts/capture.py --effect junoChorus --signal broadband --seconds 12 \
  --seed 42 --param0 1 --param1 0.5 --param2 0.5 --block-size 128 \
  --output build/captures/juno-modeI.wav
python3 scripts/capture.py --effect passthrough --input build/captures/juno-modeI.input.wav \
  --output build/captures/passthrough.wav
python3 scripts/compare.py build/captures/passthrough.wav build/captures/juno-modeI.wav \
  --output build/captures/comparison.json
```

Use new names: captures, sidecars and listening files are never overwritten. `--signal` accepts sine, impulse or deterministic broadband. Synthetic stereo defaults to distinct channels; `--stereo mono` explicitly duplicates the stimulus. Input WAV supports PCM16/24/32 and float32 at 48000 Hz, one or two channels. Mono input is duplicated and recorded; there is no implicit sample-rate conversion. Input-file captures use its entire duration.

Unspecified knobs use the effect's metadata defaults. Values are finite normalized 0..1. Actions and parameter changes are sample-scheduled with an optional JSON file:

```json
[
  {"frame": 0, "action": 0},
  {"frame": 240001, "action": 1},
  {"frame": 240019, "parameter": 1, "value": 0.9}
]
```

Supply `--events events.json`. Actions 0/1 are official left press/hold IDs; their meaning belongs to the selected effect. The probe splits callbacks at event boundaries and applies same-frame events in file order. Compare different `--block-size` values to check partition independence. Juno press changes I/II, while hold enters/leaves I+II; neither simulates the pedal's physical right bypass.

## Measurements and listening copies

Comparison rejects unequal frame counts, differing sample rates, non-stereo files and sidecars with unsupported declared schema versions, contradictory WAV dimensions/hashes/durations, invalid parameters or out-of-range events. Compiler-generated dependencies identify shared headers and included data in capture provenance. Physical recordings without sidecars are permitted and explicitly lack metadata.

Reports include raw left/right levels, peaks, finite sample counts, samples at or above full scale, differences, gain ratios and correlation. Full-scale counts are a headroom indicator, not proof that float samples were already clipped. Mono uses `(L + R) / 2`. Stereo correlation uses the normalized raw channel inner product; other correlation measurements subtract the mean. Silent/constant signals can yield an undefined centered correlation, represented by JSON null. Spectra use the first common power-of-two window, at most 32768 frames, with a Hann window. This is a localized comparison, not whole-recording spectral evidence or an authenticity score.

Optional listening copies are separate:

```sh
python3 scripts/compare.py build/captures/passthrough.wav build/captures/juno-modeI.wav \
  --preview-dir build/captures/listening --target-dbfs -24 --shift-b 0
```

A positive shift delays a copy by inserting zeros; a negative shift advances it. Length is retained by discarding the opposite edge. Each copy gets one gain for both channels, targeting combined stereo RMS. Applied shifts/gains are saved in `transforms.json`. Raw comparison metrics remain unchanged. Targets that would clip are rejected; no limiter or automatic timing estimate is applied.

For physical comparisons, measure latency/System Delay, retain untouched recordings and label any manually chosen alignment. Longer modulation runs, level sweeps, silence/DC tests and action stress should be chosen for the effect's behavior.

## Relocation acceptance

`python3 tests/verifyMigration.py --baseline /path/to/original/PatchImpl.cpp` compares the original and current Juno code at 12 seconds in all modes and interrupted transitions, including callbacks of 64 and 127 samples. The original header must be adjacent to the original source. This is a one-time migration verification tool, not a requirement to keep old source duplicates in the repository. Results and WAV hashes live under ignored `build/migration-baseline/`.
