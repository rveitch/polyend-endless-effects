# Polyend Endless platform reference

Audit date: **2026-09-19**. This reference separates the public SDK contract from documented hardware behavior and facts that still require hardware verification.

## Verified SDK revision

The public `polyend/FxPatchSDK` repository has one branch, `master`, at **`708f08d7c8e365b8a153c66e0e5200fbdaff1ce0`**, committed **2026-03-17 14:28:51 UTC**. The local SDK at `/Users/ryanveitch/nodejs/polyend/FxPatchSDK` matches that revision. GitHub API checks found five commits, no tags, and no releases. There have been **no public SDK commits since April or May 2026** as of this audit. This does not establish whether device firmware or Playground has changed.

Sources: [commit history](https://github.com/polyend/FxPatchSDK/commits/master/), [pinned revision](https://github.com/polyend/FxPatchSDK/commit/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0), [branches API](https://api.github.com/repos/polyend/FxPatchSDK/branches), [releases API](https://api.github.com/repos/polyend/FxPatchSDK/releases), [tags API](https://api.github.com/repos/polyend/FxPatchSDK/tags).

Relevant March 17 changes:

| Commit | Change |
| --- | --- |
| `7df0cacd47d391ba40de4f46e869c4252225a4fb` | Corrected working-buffer size reporting by removing an erroneous extra multiplication by `sizeof(float)`. |
| `1a936323d33175c75acac12f88c7d68be1bcff12` | Added configurable `PATCH_NAME` and changed output extension from `.bin` to `.endl`. |
| `708f08d7c8e365b8a153c66e0e5200fbdaff1ce0` | Removed `Patch::isParamEnabled` and `ParamSource`; expression assignment is now selected in hardware. |

## Official manual and hardware controls

The public [Endless downloads page](https://polyend.com/downloads/endless-downloads/) lists **Quick Start Guide 1.0**. Its actual download is the [official six-page PDF](https://polyend-website.fra1.digitaloceanspaces.com/wp-content/uploads/2026/03/17114232/endless_guide_web.pdf). Page references below count the cover as page 1. This is a quick start guide, not a comprehensive DSP or firmware manual. The PDF and downloads HTML were retrieved successfully directly, despite errors opening them through the web research tool.

| Control or behavior | Documented contract | PDF page |
| --- | --- | --- |
| Left footswitch | Customizable press and hold actions. | 2 |
| Right footswitch | On/off, meaning bypass. | 2 |
| Secondary controls | Hold right footswitch while adjusting knobs to access Input, Output, and Mix; release to return to primary controls. Available for every effect. | 3 |
| Mono input | Hold left footswitch while powering on. | 4 |
| Stereo input | Hold right footswitch while powering on. | 4 |
| Audio connectors | TRS mono/stereo-compatible input and TRS stereo output; 24-bit/48 kHz audio. | 2 |
| Expression assignment | Hold right, then hold left; after magenta pulsing, turn the knob to assign. | 4 |
| Patch installation | Copy one `.endl` onto the `Endless` USB drive. Upload replaces the currently installed effect. Flashing yellow means installing, green success, red failure. | 5 |
| Firmware update | Press the update button between USB and power until blue blinking; connect USB and copy firmware to `Endless FWU`; completion is green followed by white On light. | 6 |

The startup setting explicitly concerns **input** mode. Do not describe it as a documented patch-level mono/stereo output selector. The guide does not specify how firmware maps mono input into the two DSP buffers or the exact global Mix law and signal-path ordering. Those require testing or further manufacturer documentation.

The expression instructions include a caution about entering another mode if left is held first. Follow the documented right-then-left sequence. The guide does not document expression persistence, calibration ranges, or its precise relationship to `setParamValue` calls.

## Patch API and action semantics

[Pinned `source/Patch.h`](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/source/Patch.h) defines:

- Three knob parameters: `kParamLeft = 0`, `kParamMid = 1`, `kParamRight = 2`.
- Two action IDs: **`kLeftFootSwitchPress = 0` and `kLeftFootSwitchHold = 1`**. They are two gestures of the left switch, not two physical switches.
- `init()`, `setWorkingBuffer(...)`, `processAudio(left, right)`, `getParameterMetadata(...)`, `setParamValue(...)`, `handleAction(...)`, and `getStateLedColor()`.
- Equal-sized left/right audio spans. No fixed callback block size is promised.
- `setParamValue` runs on the audio thread. The same thread guarantee is not explicitly documented for `handleAction` or LED polling.

The [C++ wrapper](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/internal/PatchCppWrapper.cpp#L53-L62) enables parameter IDs 0 through 2 only when source ID is 0, identifying knobs. It delegates action IDs directly to `handleAction`. The current C++ interface has no `isParamEnabled`, `ParamSource`, bypass callback, right-foot action, raw switch state, or explicit release callback.

**Unspecified:** hold threshold; whether an action fires on press or release; whether a hold also causes the press action; action callback threading; bypass reset, tail, and scheduling behavior. Do not build behavior around guessed event semantics. Verify these on hardware before relying on them.

The underlying [C binary ABI](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/internal/PatchABI.h#L14-L18) remains **`PATCH_ABI_VERSION = 0x000B`** with magic `0x48435450` (`PTCH`). C++ interface changes and firmware ABI version are distinct concerns.

## Build artifacts and resource limits

The [Makefile](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/Makefile#L50-L51) builds `build/<PATCH_NAME>.elf` and `build/<PATCH_NAME>_<timestamp>.endl`. The `.endl` is the raw binary produced by objcopy. The README deployment section and ABI header comment still say `.bin`; those references are stale. Use `.endl`, consistent with the Makefile and manual page 5.

| Resource | Verified value or restriction | Source |
| --- | --- | --- |
| Processor | 720 MHz ARM Cortex-M7 | [Official product page](https://polyend.com/endless/) |
| Sample rate | 48,000 Hz | [Patch.h](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/source/Patch.h#L35-L37) |
| Working buffer | 2,400,000 floats, approximately 9.6 MB decimal; external memory with higher access latency | [Patch.h](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/source/Patch.h#L76-L81) |
| Buffer-size ABI units | Float count, not byte count | [Wrapper](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/internal/PatchCppWrapper.cpp#L23-L26) |
| Patch code/data/BSS region | 512 KiB at default `0x80000000`, separate from host-supplied working buffer | [Linker script](https://github.com/polyend/FxPatchSDK/blob/708f08d7c8e365b8a153c66e0e5200fbdaff1ce0/internal/patch_imx.ld#L1-L8) |
| Allocation | No heap, malloc, or dynamic memory allocation; use patch members or supplied buffer | [README](https://github.com/polyend/FxPatchSDK#develop) |
| Output bounds | Keep samples within `(-1.0f, 1.0f)` to avoid digital clipping | [README](https://github.com/polyend/FxPatchSDK#develop) |
| Execution deadline | Excessive time in audio processing or other patch methods causes dropped frames and artifacts | [README](https://github.com/polyend/FxPatchSDK#develop) |

The public SDK supplies no exact CPU percentage budget, callback frame count, usable stack limit, latency guarantee, or portable performance benchmark. Host-side tests can validate DSP behavior but cannot establish real-time safety on the pedal.

## Firmware and recovery: verified limits

No current Endless firmware version or release changelog was established. The public downloads HTML listed only the quick start guide on the audit date; searches did not locate an official Endless firmware release announcement. This is not evidence that firmware has remained unchanged. Account gating was not established either, and no account-only firmware content or device-installed version was inspected.

An April 16–17, 2026 [official-community recovery thread](https://backstage.polyend.com/t/de-brick-guide-for-endless-is-required/24508) reports missing public firmware files and a confirmed recovery method: **hold both footswitches while powering on to clear the active effect**. This clears the installed effect and is recovery guidance, not a routine validation step. No hardware reset, upload, or firmware update was performed during this audit.

Before release, record the actual device firmware version if accessible, verify input routing and global Mix interaction, observe short/hold event sequences, test bypass transitions and tails, and measure worst-case processing behavior on hardware. These are outstanding validation tasks, not claims that the current implementation fails.
