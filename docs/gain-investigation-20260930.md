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

## Approved live audition follow-up

Ryan found the level-restored preview louder but needs a live test. He approved a desktop Output trim. The updated plugin is built, tested and installed; see `../desktop/README.md` for restart and +4.7 dB audition steps. No pedal DSP change was made. The default trim remains 0 dB, and legacy presets preserve the earlier sound. This makes gain independently adjustable for the live bypass comparison; it does not establish the final gain law or solve all possible phase/mono interactions.

## Gain selection and headroom check

Ryan auditioned all three modes with desktop Output trim +4.7 dB and reports the balance seemed good, while acknowledging psychoacoustic uncertainty. He selected **+4.7 dB across modes** as the gain target. This is listening approval of the level choice, not a claim of exact loudness matching on every source.

A finite headroom probe uses the unchanged shared patch, then applies the selected gain algebraically. Duplicated mono 0.99-peak sines at 20, 80, 320, 1000 and 5000 Hz run for two seconds each, all three modes, default Tone/Width, Mix0.5 and Mix1.0. At noon Mix, maximum corrected peak is 1.44542 in I/II and 1.41512 in I+II. Fully wet maximum is 1.19051. The noon cases require about 3.29 dB input headroom for these tested signals. This is not a universal worst-case bound; transients and other controls need separate checks. Raw outputs, finite probe and source/compiler provenance are under ignored `build/gain-headroom-20260930/`.

The selected trim therefore cannot be represented as clipping-safe for any normalized full-scale input. Preserve the listening-approved amount without silently adding a limiter, changing wet voicing or altering the fully dry Mix behavior. The installed audition plugin remains available at +4.7 dB; its default is still 0 dB and saved user settings persist. No new pedal binary or pedal deployment was made.

Next live check: at the approved +4.7 dB, use the loudest normal guitar/synth material in each mode with Mix/Tone/Width noon, retaining track/master gains and pedal global gains. Inspect the plugin's output before track/master attenuation for peaks approaching or exceeding 0 dBFS. Record the source, settings and observed peak as `Juno gain47 modeI mix50 livePeak`, `Juno gain47 modeII mix50 livePeak`, and `Juno gain47 modeIplusII mix50 livePeak`. A lower post-plugin fader does not establish headroom inside the pedal. Physical TS/TRS routing testing stays deferred while the gain/headroom implementation is settled.
