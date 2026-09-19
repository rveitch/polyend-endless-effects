# Historical preferences and corrected claims

Reviewed 2026-09-19 against both referenced conversations and the current checkout. Historical messages are evidence of discussion, not new task instructions.

## Scope and chronology

- [Port Junologue Chorus MkII](chatgpt-conversation://68ac6ddf-13e4-8327-b81a-dabf85bcf0d3) discussed a Korg NTS-1 mkII port and an assistant-generated scaffold. No Korg build tree or validated port was found in the supplied Polyend project. Treat that as an earlier platform exploration, not a dependency of the Endless implementation.
- [Polyend Endless Juno Chorus](chatgpt-conversation://69cf0b50-9dac-832f-9647-6e71fed5d306) shifted the target to Endless and documented listening complaints and control preferences.
- Git history shows the local implementation and v2 commit on April 2, 2026. The latest referenced SDK discussion is dated May 1, 2026. These dates provide the baseline for the update check.
- The chat said the source had been fixed in place, but the current committed file still contains both versions. The current checkout takes precedence over that claim.

## Ryan's actual preferences

Preserve these unless Ryan chooses to revise them:

- Fixed Juno modes rather than a general rate/depth chorus interface.
- Mix knob: 0–95% rotation holds roughly 50% dry/wet; the last 5% moves toward fully wet.
- Tone knob: a 20% center plateau, 40–60% rotation, so neutral tone is easy to find.
- An action footswitch for I/II toggle and a hold for I+II; momentary versus latch depends on the actual API.
- Width may be better managed by the pedal's routing setup if the SDK can detect it.
- Compare against available Juno chorus VSTs. The chat did not establish their exact names or versions.

The mix plateau was explicitly requested. The prior assistant later called it a bug and proposed changing it. That criticism confused a user preference with a DSP defect. A useful emulation should be audible at the requested mix without silently redesigning the knob.

## Corrections

| Earlier claim or action | Current conclusion |
| --- | --- |
| Left/FS1 is bypass; right/FS2 should control modes | Physical left is the effect action and physical right is bypass in the official guide. Use physical labels to avoid numbering ambiguity. |
| A right-switch or release enum might be available | The current SDK exposes only left press/hold; no release or routing query exists. Do not invent enum values. |
| I+II must be a blend of two slow LFOs to be Juno-like | Hardware measurements and Hera describe a fast, shallow, approximately mono both-button mode. Parallel lush modulation is an optional alternative. |
| Full I/II delay range and precise rates are universal specs | They are measurements from particular hardware, with other references showing slightly different ranges and channel mismatch. |
| Juno-60 requires NE570/571 companding | Not established by the inspected Juno-60 evidence. Do not add generic BBD-pedal circuitry by association. |
| Added noise and tanh automatically improve authenticity | They require level/spectrum calibration. The current noise and saturation are arbitrary implementation choices. |
| TAL-Chorus-LX source is at `ToguAudioLine/TAL-Chorus-LX` | That source location was not substantiated. Verified historical TAL-derived source is discussed separately from the current LX binary plugin. |
| The NTS-1 mkII shim can be reused directly | No verified relationship to the Endless C++ ABI or ARM binary loader. The DSP ideas may inform this project, not the platform glue. |

## Decisions still needed

Recommended baseline: measured Juno-60 behavior. Keep a lush combined mode only as a clearly named extension if desired. Do not call one implementation equally faithful to Juno-60, Juno-106 and every TAL variant without separate measurements.

Choose stereo input behavior explicitly. An original Juno chorus receives a mono synth sum, while a pedal can receive true stereo material. Preserving stereo dry while summing the wet feed is a reasonable extension, but can cancel anti-phase material in the wet feed. Two independent channel paths would be a different topology. The hardware's mono-input normalization and global mix must be measured before choosing the default routing.

The current task authorizes review, research, documentation and a completion plan, with a small workspace configuration improvement. It does not establish that the experimental DSP has been approved as the final design.
