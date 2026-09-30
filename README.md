# Polyend Endless effects

Independent effects built against a pinned official Polyend SDK. The approved Juno chorus candidate and unity passthrough diagnostic live in `effects/`. The Juno candidate compiles and passes host tests; its provisional wet voicing still needs pedal validation.

## Build and test

Requires Python 3.12 or newer, GNU Make, a C++20 host compiler, and the Arm GNU toolchain for device builds. No sibling SDK checkout or submodule initialization is needed.

```sh
make list
make test EFFECT=all
make test-tools
make build EFFECT=all TOOLCHAIN=/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi-
make check EFFECT=all
```

Use your own compiler prefix on other machines. Device outputs go to ignored `build/<effectId>/`: timestamped `.endl` files with preserved matching ELFs and `.manifest.json` provenance. Dependency-content hashes prevent stale builds even when file timestamps are unchanged. Builds inspect the image header, callback addresses and RAM bounds before reporting success. Host tests and structural checks do not establish pedal CPU performance or hardware authenticity.

```sh
make build EFFECT=junoChorus PATCH_NAME=juno_chorus TOOLCHAIN=/path/to/bin/arm-none-eabi-
make test EFFECT=passthrough
```

Old `make -C example` and `make -C example -f diagnostic/Makefile` commands still delegate to the selected effect. Saved binaries and historical captures are preserved. Root `make clean EFFECT=<id>` removes only that effect's default generated directory; legacy `clean` preserves saved outputs.

## Workflows

- [Add an effect](docs/effect-authoring.md): explicit registration, controls, buffer use and validation.
- [SDK maintenance](docs/sdk-maintenance.md): check upstream, review changes, explicitly import, then rebuild and compare.
- [Stereo captures and comparison](docs/audio-workflow.md): reproducible float WAVs, sample-scheduled controls and raw L/R/mono measurements.
- [Migration evidence](docs/repository-migration-20260930.md): preserved behavior and verification limits.
- [Desktop audition evaluation](docs/desktop-audition-evaluation.md): experimental wrapper assessment and macOS prerequisites.
- [Project notebook](docs/README.md): Juno research, dated measurements and current candidate handoff.

`vendor/FxPatchSDK/` is an unchanged official snapshot. `sdk.lock.json` records its full upstream commit and file hashes. Project build rules belong in `buildSupport/`; effects are selected through `effects/catalog.json`. Do not modify the vendor snapshot to add effects.
