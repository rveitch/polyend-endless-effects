# Juno chorus candidate

The approved 2026-09-29 vintage candidate, preserved during repository organization. See [the candidate record](../../docs/vintage-candidate-20260929.md) for voicing, hypotheses and next pedal measurements.

```sh
make build EFFECT=junoChorus TOOLCHAIN=/path/to/bin/arm-none-eabi-
make test EFFECT=junoChorus
```

Left: diagnostic Mix, with 0-5% dry, noon 50/50 and maximum fully wet. Middle: Tone. Right: stereo Width, reaching full width at 40%. Left-foot-switch press switches Modes I/II; hold enters/leaves I+II. Physical right bypass belongs to the pedal.

Blue I+II now uses the user-approved slow TAL-inspired stereo sweep (about 0.4 Hz). Red and green retain their accepted motion. See [the blue revision record](../../docs/blue-tal-20261002.md). Wet filtering and gain are provisional voicing choices. Current host tests cover Mix and candidate behavior; device authenticity and pedal CPU validation remain outstanding. The migration changed include paths and organization only.

Capture with [the stereo workflow](../../docs/audio-workflow.md). Mode-specific actions must be scheduled explicitly; a fresh instance starts in Mode I. Existing `example/source/PatchImpl.cpp` and old test paths forward here for compatibility.

## Gain-corrected pedal test, 2026-10-02

The pedal test candidate now includes +4.7 dB after the complete blend at Mix noon and above, all modes. It approaches unity continuously below noon and preserves the true-dry zone. Desktop builds retain the separate Output trim without applying this correction twice. See [artifact, controls and headroom evidence](../../docs/pedal-gain47-20261002.md). This linear boost requires input headroom; it is not a full-scale-safe release. The gain correction leaves filters and modulation unchanged; the subsequent blue-only revision is documented separately.
