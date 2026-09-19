# Public chorus implementation references

Checked 2026-09-19. Source inspection is not a build, audio-quality test, or performance benchmark.

## Hera: useful clock-driven research implementation

Repository: [jpcima/Hera](https://github.com/jpcima/Hera). Inspected master commit `f6fe5b900f4cf84809686466e0a37de5edf008fd`.

[HeraChorus.dsp](https://github.com/jpcima/Hera/blob/f6fe5b900f4cf84809686466e0a37de5edf008fd/Source/HeraChorus.dsp#L17-L49) defines:

| Mode | Rate | Left delay | Right delay | Modulation |
| --- | --- | --- | --- | --- |
| I | 0.513 Hz | 1.54 to 5.15 ms | 1.51 to 5.40 ms | Opposing triangle |
| II | 0.863 Hz | 1.54 to 5.15 ms | 1.51 to 5.40 ms | Opposing triangle |
| I+II | 9.75 Hz | 3.22 to 3.56 ms | 3.28 to 3.65 ms | Common sine-like |

[Lines 66-98](https://github.com/jpcima/Hera/blob/f6fe5b900f4cf84809686466e0a37de5edf008fd/Source/HeraChorus.dsp#L66-L98) use 0.83 direct gain, a shared LFO and its inverse, and choose `analogDelayModel`. The alternative digital model has fourth-order 10 kHz low-pass filters before and after the delay. That alternate filter choice does not describe the active analog model or prove the hardware response.

[bbd_line.cpp lines 68-112](https://github.com/jpcima/Hera/blob/f6fe5b900f4cf84809686466e0a37de5edf008fd/Source/bbd_line.cpp#L68-L112) accumulate fractional clock phase, process every elapsed tick, and interpolate input/output filter responses at tick times. This is substantially more than a fractional read from an ordinary sample-rate delay. [bbd_filter.cpp lines 117-127](https://github.com/jpcima/Hera/blob/f6fe5b900f4cf84809686466e0a37de5edf008fd/Source/bbd_filter.cpp#L117-L127) define five-pole input/output residue and pole tables named for J60. Their existence is verified; their complete derivation and fit error were not established in this pass.

Licensing for reuse: these three inspected chorus/BBD source files explicitly identify ISC. The [overall README](https://github.com/jpcima/Hera/blob/f6fe5b900f4cf84809686466e0a37de5edf008fd/README.md) says the complete synthesizer is GPL and alpha quality, with known inaccuracies. Preserve per-file notices and review dependencies when extracting anything; do not label the entire repository permissively licensed or a validated hardware clone.

## Junologue: compact port example with deliberate differences

Repository: [peterall/junologue-chorus](https://github.com/peterall/junologue-chorus), main commit `88bd17537afbe305e1b7d6476732ff09a9eae46d`.

The [README](https://github.com/peterall/junologue-chorus/blob/88bd17537afbe305e1b7d6476732ff09a9eae46d/README.md) explicitly distinguishes its parallel I-II mode from the original both-buttons behavior. It targets Korg Prologue, Minilogue XD and NTS-1. Its Speed control combines mode selection and brightness; Depth controls wet/dry.

Inspected [junologue-chorus.cpp](https://github.com/peterall/junologue-chorus/blob/88bd17537afbe305e1b7d6476732ff09a9eae46d/junologue-chorus.cpp):

- Lines 9, 25-32: fixed 48 kHz, per-side delay ranges matching Hera's normal modes, rates 0.513/0.863 Hz.
- Lines 119-127: linear interpolation of delay time between endpoints, using `p` and `1-p` for opposite sides.
- Lines 151-167: stereo input sum through a soft limiter and prefilter; both triangle LFOs run, and their delayed results are weighted and combined before postfiltering and dry/wet mixing.
- Lines 84-93 and 130-131: first-order low-pass methods are used. The nearby comment mentioning 12 dB is not the implementation contract.
- Lines 17-20: middle mode weights both rates at approximately -3 dB each. It contains no 9.75 Hz authentic fast-mode branch.

The [project license](https://github.com/peterall/junologue-chorus/blob/88bd17537afbe305e1b7d6476732ff09a9eae46d/LICENSE) grants MIT-style permission with notice retention. SDK and incorporated helper origins still need their own notice review if reused. It is a useful small embedded integration example, not evidence that Korg binaries or hooks work on another platform.

## TAL: keep the product and public DSP lineage separate

The [official TAL-Chorus-LX page](https://tal-software.com/products/tal-chorus-lx) describes a Juno-60 emulation and supplies binary downloads. No source download or official source repository was identified there. Do not invent or cite a `ToguAudioLine/TAL-Chorus-LX` repository as established source access.

[SpotlightKid/ykchorus](https://github.com/SpotlightKid/ykchorus) identifies its engine as extracted from TAL-NoiseMaker and states GPL-2.0. [soerenbnoergaard/talchorus](https://github.com/soerenbnoergaard/talchorus) likewise describes an extraction from NoiseMaker. Both public engines were inspected below. This does not establish identity with the current Chorus-LX engine. Treat them as a separate GPL code path if actual reuse is proposed. No current build or audio benchmark was performed.

### YKChorus engine inspection

Pinned master: `ee0e362f8ef30b55a440715e4da3bfb3dad6a30f`.

[ChorusEngine.h lines 85-110](https://github.com/SpotlightKid/ykchorus/blob/ee0e362f8ef30b55a440715e4da3bfb3dad6a30f/plugins/YKChorus/ChorusEngine.h#L85-L110) instantiate four delay objects, a left/right pair at 0.5 Hz and another at 0.83 Hz. Within each pair the initial triangle phases oppose one another. Type I and II select pairs, while both-enabled runs both pairs and sums their processed outputs. Each side consumes its own stereo input; there is no mono fold-down. The final engine mix is original input plus 1.4 times the accumulated wet result. DC blocking is applied after the first enabled addition and after the second addition, so in combined mode the accumulated first contribution also passes through the second DC blocker.

[Chorus.h lines 55-63, 97-138](https://github.com/SpotlightKid/ykchorus/blob/ee0e362f8ef30b55a440715e4da3bfb3dad6a30f/plugins/YKChorus/Chorus.h#L55-L138) advance a triangle by `4*rate/sampleRate` and reverse its slope at +/-1. The nominal tap is `(0.3*triangle+0.4)*7 ms`, which is approximately **0.7 to 4.9 ms**, not a fixed 7 ms delay and not the measured 1.66 to 5.35 ms range. Finite-step endpoint overshoot and the interpolation recurrence can slightly affect actual response.

The interpolation is recursive: with fractional position `f`, its output is the older tap plus `(1-f)` times the newer tap minus `(1-f)` times the previous output. This is an allpass-style fractional interpolation recurrence, not the ordinary linear interpolation in this project's code. Output filtering calls `OnePoleLP` with a fixed value 0.95; [OnePoleLP.h lines 37-40](https://github.com/SpotlightKid/ykchorus/blob/ee0e362f8ef30b55a440715e4da3bfb3dad6a30f/plugins/YKChorus/OnePoleLP.h#L37-L40) turn that into pole `p=(0.95*0.98)^4`, approximately 0.751275. The recurrence is `y=(1-p)*x+p*yPrevious`. Calculating its discrete-time half-power point gives approximately **2.20 kHz at 48 kHz**, with cutoff scaling with sample rate. The argument is not a frequency in hertz. There is no inspected pre-BBD anti-alias filter, explicit BBD tick simulation, compander, noise generator, or saturation in these engine paths.

[PluginYKChorus.cpp lines 124-165](https://github.com/SpotlightKid/ykchorus/blob/ee0e362f8ef30b55a440715e4da3bfb3dad6a30f/plugins/YKChorus/PluginYKChorus.cpp#L124-L165) divide exposed rate values by ten. Defaults 5.0 and 8.3 therefore mean 0.5 and 0.83 Hz, not 5 and 8.3 Hz. This wrapper-level conversion is easy to misread when comparing presets.

### Independent talchorus source path

Pinned master: `bb9bfbfa76d3c6a86379ce292ba5926b2d9f3154`. Its [wrapper](https://github.com/soerenbnoergaard/talchorus/blob/bb9bfbfa76d3c6a86379ce292ba5926b2d9f3154/src/talchorus/TalChorus.cpp#L174-L226) forwards stereo samples and two enable flags to `ChorusEngine`. The DSP comes from its pinned `distrho-ports` submodule, commit `65c7c68a79e532d01695466f5b94c0e1cc4ae940`, not an unpinned present-day NoiseMaker download.

Inspected [that engine](https://github.com/DISTRHO/DISTRHO-Ports/blob/65c7c68a79e532d01695466f5b94c0e1cc4ae940/ports/tal-noisemaker/source/Effects/Chorus/ChorusEngine.h#L83-L110), [delay implementation](https://github.com/DISTRHO/DISTRHO-Ports/blob/65c7c68a79e532d01695466f5b94c0e1cc4ae940/ports/tal-noisemaker/source/Effects/Chorus/Chorus.h#L103-L145), and `OnePoleLP.h`. The relevant rates, four-delay topology, 7 ms scale with 0.3/0.4 mapping, recursive interpolation, fixed 0.95 filtering argument and 1.4 wet gain agree with YKChorus. They are two integrations of the same public lineage, not two independent validations of hardware behavior. The wrapper's permissive header does not change the TAL engine's GPL-2.0 terms.

### Comparison with this project's two baseline blocks

Comparison is against `example/source/PatchImpl.cpp` at baseline `bc31ea43a596f6f19fcffdaf1db41efd5614376e`, before any repair. Neither block is equivalent to the public TAL engine:

| Aspect | Public NoiseMaker-derived engine | Project baseline |
| --- | --- | --- |
| Both-enabled topology | Adds two pairs of separately delayed signals | Later block blends LFO values 60/40 before one delay pair |
| Authentic fast mode | Absent | First block selects 9.75 Hz but keeps triangle and width inversion |
| Delay trajectory | Roughly 0.7 to 4.9 ms | First block 1.66 to 5.35 ms; later normal-mode ranges differ |
| Fractional delay | Recursive allpass-style | Linear interpolation |
| Input/dry routing | Each original stereo side retained | Both blocks average input to mono and filter the dry branch |
| Wet color | Fixed one-pole output filter and DC blockers | Pre/post low-pass, arbitrary noise and tanh |
| Mixing | Dry + 1.4*wet | Nominal 50/50 with special upper mix range |

The later block's blended modulation is also different from Junologue's parallel delayed-signal sum. In general, reading once at a weighted average delay does not equal averaging signals read at two separate delays. TAL provenance cannot justify that topology as either TAL-equivalent or original-Juno behavior.

## Upstream chronology

Dates below are verified GitHub commit metadata for the checked default-branch tips, not release dates or promises that every related repository is inactive:

| Repository | Tip commit date, UTC | Tip subject |
| --- | --- | --- |
| [Hera](https://github.com/jpcima/Hera/commit/f6fe5b900f4cf84809686466e0a37de5edf008fd) | 2021-08-15 | Try adding Linux to workflows (6) |
| [Junologue](https://github.com/peterall/junologue-chorus/commit/88bd17537afbe305e1b7d6476732ff09a9eae46d) | 2020-10-28 | Remove unused code, add optimizations to C objects, update build logging |
| [YKChorus](https://github.com/SpotlightKid/ykchorus/commit/ee0e362f8ef30b55a440715e4da3bfb3dad6a30f) | 2024-05-27 | feat: don't compile in support for ALSA/Pulseaudio/SDL in stand-alone by default |
| [talchorus](https://github.com/soerenbnoergaard/talchorus/commit/bb9bfbfa76d3c6a86379ce292ba5926b2d9f3154) | 2020-02-16 | Commit date verified; subject not needed for DSP analysis |

These tips predate April/May 2026. The stronger findings here come from examining existing source more carefully, not discovering a post-May revision of these engines. The BBD LFO application note was updated February 2026, also before that period.

## Reference ranking

Use Roland circuit drawings to establish architecture, recordings to calibrate behavior, Hera to study detailed BBD processing, and Junologue to understand a compact embedded approximation. Use TAL-Chorus-LX as an audible comparison if available, not as an assumed public source dependency. These sources complement one another; none alone proves target-device performance or exact hardware matching.
