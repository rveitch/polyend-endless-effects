# Mode I plugin reference captures, 2026-09-29

All four plugins rendered the same existing hardware-test stimuli at 48 kHz, stereo, 74 seconds. Tone occupies 0-34 seconds (active 2-32), broadband occupies 40-74 seconds (active 42-72). Tone item gain is -24 dB, giving -48 dBFS RMS stimulus; broadband item gain is unity. No normalization or time shifting was applied. Track and master faders remain at unity. Each plugin was routed alone to the master during its offline render; all track master sends were disabled afterward.

Files and provenance live in `/Users/ryanveitch/Documents/REAPER Media/Polyend Endless/Chorus Capture 20260928/Plugin References/`:

- `plugin-versions.json`, `initial-plugin-parameters.json`, `modeI-parameters.json`
- `modeI-render-analysis.json`: filenames, SHA-256 hashes and first-pass measurements
- `Juno Chorus References.RPP`: saved session

Versions: Roland 1.0.4, AIR 1.2.1.14, Arturia 1.5.1.6566, TAL 1.6.3.

## Settings and validity

All Mode I, plugin Mix fully wet, factory output controls retained. Roland noise level and AIR noise switch were set to zero. TAL width remains full, compatibility parameter zero. Arturia On/Off remains at its initial zero value; the supplied UI shows illuminated power and the rendered signal is modulated, so zero must not be interpreted as proof of bypass. Input Mode remains at its initial value; identical mono source feeds both host channels.

The first four `take1.wav` exports were completely silent and are INVALID. REAPER showed media offline. Bringing media online with action 40101 restored audio. The four `take2.wav` files are the valid captures. Never include take1 files in comparisons. No raw files were deleted or overwritten.

## Initial measurements

Analysis windows: tone 3-31 seconds, broadband 43-71 seconds. RMS is raw per-channel dBFS. Modulation frequency below is the dominant periodogram bin of detrended, unwrapped 1 kHz output phase, with 1/28 Hz resolution. It is preliminary, not a fitted precision LFO estimate.

| Plugin | Broadband L / R RMS dBFS | L/R correlation | Approximate modulation Hz |
|---|---:|---:|---:|
| AIR Jura | -60.373 / -60.373 | 0.0067 | 0.500 |
| Arturia JUN-6 | -66.909 / -66.907 | 0.0072 | 0.393 |
| Roland | -63.796 / -63.797 | 0.0055 | 0.464 |
| TAL-Chorus-LX | -62.088 / -62.088 | 0.0031 | 0.393 |

All four are nonzero, stereo, 48 kHz and exactly 74 seconds, with peaks below -40 dBFS. Zero-lag projection onto the original broadband source is below 0.002 in magnitude for every channel, consistent with removal of undelayed dry audio. This alone does not rule out a delayed dry component or establish plugin latency. Explicit dry-path/PDC checks remain necessary before precise delay comparisons.

Output differences combine factory gain, wet-path filtering and plugin design. They are not evidence of authenticity or audible superiority. AIR activation is confirmed operationally by its nonzero modulated render. No pedal settings were changed.

## Next steps

1. Capture plugin dry/bypass controls to verify gain and host latency compensation before estimating absolute modulated delay.
2. Estimate wet delay trajectories and spectral coloration, preserving raw levels and comparing hardware after its measured passthrough baseline correction.
3. Extend to Mode II and I+II, then render a common musical excerpt at each plugin's ordinary mix for listening. Do not rank authenticity by brand.
