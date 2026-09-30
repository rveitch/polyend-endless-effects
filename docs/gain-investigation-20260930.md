# Default chorus level investigation, 2026-09-30

Ryan reports that the desktop plugin sounds like the pedal to his ears, including the perceived engaged level drop. He reproduced it by toggling bypass with default controls while playing `Dry reference RMS matched preview.wav`. This is user listening evidence; monitoring configuration and quantitative live REAPER levels were not independently read back. Prioritize default gain/balance before resuming physical TS/TRS stereo/mono tests.

## Reproduction

Current source at commit `359a29b`, official pinned SDK, finite host render with the same dry preview, 48 kHz, callbacks 128, Mix/Tone/Width 0.5. Mode actions select I, II or I+II in fresh renders. Combined stereo RMS measured over the active 2..32 seconds. Input active RMS approximately -20 dBFS. This directly exercises the shared patch code; it is not a new recording of Ryan's live REAPER toggle.

| Mode | Current output versus dry | Removing wet attenuation only |
| --- | --- | --- |
| I, default | -4.698 dB | -3.450 dB |
| II | -4.718 dB | -3.471 dB |
| I+II | -4.288 dB | -3.011 dB |

Current code multiplies filtered wet by 0.7 for provisional headroom, then at noon sums 0.5 dry and 0.5 attenuated wet. Wet RMS here is about -3.112 dB versus dry. Dry/wet correlation is -0.0948 in I, -0.0991 in II and 0.00142 in I+II. Dry/wet phase relationship matters: two mostly uncorrelated equal-level signals mixed at half amplitude do not preserve the original signal power.

The wet-attenuation counterfactual is reconstructed algebraically from the current render as wet = 2 * output - dry, then output = 0.5 * dry + 0.5 * wet / 0.7. It changes only wet amplitude. It is an analytical comparison, not a changed installed effect or a claim of universal gain behavior.

## Controlled listening comparison

Ignored `build/gain-investigation-20260930/` retains the finite measurement script, original-current WAVs, compiler/dependency provenance, source hash, results and listening transform. `I-mix50-output-level-restored.wav` adds a fixed +4.698 dB to both channels of the default-mode output, matching input stereo RMS on this music window. Peak is -8.528 dBFS. There is no EQ, limiting, alignment or change to wet balance/modulation. Raw current render remains untouched.

Compare the dry preview, `I-mix50-current.wav` and `I-mix50-output-level-restored.wav` at unchanged playback gain. If restored output resolves the perceived loss while keeping the desired chorus amount, investigate a mix-dependent output gain law with headroom validation. If it still feels weak/thin, quantify frequency-dependent cancellation and wet balance separately before changing filters. +4.698 dB is clip-specific diagnostic compensation, not an approved fixed boost for every signal/mix position. No pedal setting, DSP source or installed plugin was changed. Physical routing tests are deferred, not considered validated.
