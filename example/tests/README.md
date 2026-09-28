# Temporary chorus Mix diagnostic (2026-09-28)

Approved test mapping: knob 0 through 5% is exact dry; 5 through 50% ramps
linearly from 0 to 50% wet; 50 through 100% ramps to fully wet. Original left
and right samples supply the dry path, without averaging, noise or filtering.
The wet algorithm still runs at dry settings to preserve its evolving state.
No control smoothing has been added; the mapping is continuous in position.

This is a testing control, not a final-product decision. Ryan prefers a mostly
static final chorus balance. The previous mapping was 50/50 below 95%, then a
ramp to fully wet across the final 5%. Do not treat that balance as verified
Juno hardware behavior. Other known wet DSP issues remain outside this change.

From the repository root:

```sh
c++ -std=c++20 -Wall -Wextra -Werror example/tests/ChorusMixTest.cpp -o /tmp/endless-chorus-mix-test
/tmp/endless-chorus-mix-test
make -C example TOOLCHAIN=/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi- BUILD_DIR=build-chorus-mix-test PATCH_NAME=juno_chorus_mix_test all
```

Use a fresh build directory or clean this dedicated directory after header changes.
The test compares original stereo samples and a synchronized full-wet reference
across 40 blocks, checks the dead-zone endpoints and intermediate Mix positions,
and exits. The target build retains the SDK's RWX LOAD-segment linker warning.

Initial hardware captures, with this binary loaded, Mode I, Tone/Width at noon,
Mix fully counterclockwise, and the established TS wiring/REW settings unchanged:

- `Endless mixTest I bypass L 48k take1`
- `Endless mixTest I bypass L 48k take2`
- `Endless mixTest I dry0 L 48k take1`
- `Endless mixTest I dry0 L 48k take2`

The dry0 captures are engaged. The initial captures still contained chorus;
after a full Mix knob sweep and return to minimum, the following captures
matched bypass:

- `Endless mixTest I dry0 knobReset L 48k take1`
- `Endless mixTest I dry0 knobReset L 48k take2`

Measured 2026-09-28 via REW API: approximately -0.00022 dB at 1 kHz versus
the average bypass response; maximum absolute difference 0.042 dB across
20 Hz to 20 kHz using 1/48-octave smoothing; repeat difference at most
0.0122 dB. Both delays were 8.6509 ms relative to channel 3 and reported
SNR was approximately 83 dB. This verifies the left dry path for this setup,
not yet the right channel or the physical 5% boundary.

Tested binary: `juno_chorus_mix_test_20260928_152517.endl`.
SHA-256: `a2394ba05edd26067456d7a50a0c48fabeac5d844a1b6c62e41cf64678804268`.
Raw measurements are in `/Users/ryanveitch/REW Measurements/`.

The patch initializes Mix to 0.5 until a parameter update. The successful knob
sweep supports an initialization/update explanation for the first captures;
it does not prove universal firmware behavior. After loading a binary, sweep
each knob and then set the requested position before recording. Preserve raw
captures, zero timing offset, and no calibration or level normalization.
