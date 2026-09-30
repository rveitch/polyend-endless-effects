# Effects Repository Implementation Plan

> For agentic workers: use superpowers:executing-plans to implement this plan task by task in the current checkout. Ryan explicitly approved implementation on 2026-09-30.

**Goal:** Independently build and validate multiple effects against a pinned, reviewable official SDK.

**Architecture:** Vendor the official SDK unchanged. Select one effect from a catalog through project-owned build rules, retaining legacy entry points. Keep evidence tooling separate from effect DSP.

**Tech Stack:** GNU Make, C++20, existing ARM GCC 15.2.1, Python 3 standard library.

**Spec:** ../specs/2026-09-30-effects-repository-design.md

## Global Constraints

- Preserve candidate audio, controls, saved binaries, and physical capture evidence.
- SDK commit starts at `708f08d7c8e365b8a153c66e0e5200fbdaff1ce0`; official ABI remains `0x000B`.
- Stereo 48000 Hz float buffers; no heap in device effect processing.
- Host tools and finite tests may allocate. New names use camelCase where practical.
- Do not push, deploy, modify the sibling SDK, or introduce fork API extensions.
- Dirty builds are permitted, but manifests record actual dependencies and build identity.

## Review Focus

- Build selection must never link two effect singletons or reuse another effect's objects.
- An SDK/header/flag change must rebuild affected objects, not certify a stale artifact.
- Failed SDK imports and locally modified snapshots must leave the current SDK intact.
- Malformed .endl files and incompatible audio/sidecars must fail explicitly.
- Evidence processing must preserve stereo, raw timing/gain, and sample-scheduled actions.

### Task 1: Catalog, SDK snapshot, build and compatibility

**Files:** `effects/catalog.json`, `effects/{junoChorus,passthrough}/`, `vendor/FxPatchSDK/`, `sdk.lock.json`, `Makefile`, `buildSupport/arm.mk`, `scripts/effects.py`, `scripts/project.py`, legacy Makefiles/source/test shims, `tests/testBuild.py`.

**Interfaces:** `project.loadCatalog(root) -> dict`, `project.loadEffect(root, effectId) -> dict`; root CLI `list`, `build`, `test`, `check`; `buildSupport/arm.mk` consumes explicit source paths, toolchain, effect, build directory, patch name and load address.

- [ ] Capture a source-hash baseline and preserve original source/header in ignored migration evidence.
- [ ] Write CLI/build integration tests: real third fixture effect, single implementation, dependency rebuilding, unknown/path-traversal rejection, manifest identity, legacy entry points.
- [ ] Watch new catalog/build tests fail against the absent CLI.
- [ ] Import tracked SDK files from the exact official Git commit, record hashes, and relocate implementations without DSP changes.
- [ ] Implement catalog validation, isolated ARM rules with .d dependencies and build-flag invalidation, host test runner, artifact manifests, and compatibility shims.
- [ ] Run `python3 -m unittest discover -s tests -p 'testBuild.py' -v` and all existing host tests. Expected: PASS.
- [ ] Build both ARM effects using the existing toolchain and commit the deliverable.

### Task 2: SDK maintenance and artifact validation

**Files:** `scripts/sdk.py`, `scripts/endl.py`, `tests/testSdk.py`, `tests/testEndl.py`.

**Interfaces:** `sdk.verifySnapshot(root) -> dict`, CLI `verify`, `check --ref`, `update --ref --apply`; `endl.inspectImage(path, root, loadAddress) -> dict` validates against the supported pinned ABI contract.

- [ ] Write local Git fixture tests for read-only checking, additions/deletions, modified snapshot refusal, rollback on invalid/interrupted imports, lock provenance, and ABI warnings.
- [ ] Write independent binary fixtures for truncated image, bad pointers, invalid BSS, optional callbacks, RAM bounds, and unsupported format.
- [ ] Watch the SDK and inspector tests fail before implementation.
- [ ] Implement staged/rollback imports, snapshot verification, upstream change reports, and full structural inspection; wire inspection into device builds and `check`.
- [ ] Run `python3 -m unittest discover -s tests -p 'test*.py' -v`. Expected: PASS.
- [ ] Verify the official remote still resolves to the pinned revision, inspect both real artifacts, and commit.

### Task 3: Stereo captures and comparisons

**Files:** `tests/captureProbe.cpp`, `scripts/audio.py`, `scripts/capture.py`, `scripts/compare.py`, `tests/testAudio.py`, `tests/testCapture.py`.

**Interfaces:** stereo float WAV plus JSON sidecar; capture CLI accepts effect, stimulus/input WAV, normalized knobs, sample-scheduled events, callback size and duration; comparison CLI rejects incompatible input and emits raw L/R/mono and spectral metrics, with separately requested listening copies.

- [ ] Write real passthrough round-trip, deterministic stereo, action scheduling/partition, mismatched metadata, malformed WAV, correlation/mono cancellation and gain-difference tests.
- [ ] Watch each new component's tests fail before implementing it.
- [ ] Implement finite probe, standard WAV I/O, deterministic stimuli, explicit event scheduling, provenance sidecars, comparison and optional listening copies.
- [ ] Render original and relocated chorus with the same host harness for every mode at 12 seconds, distinct stereo input, multiple callback sizes and interrupted mode/Tone events. Compare float samples bit for bit.
- [ ] Run the full test suite, save migration hashes/results under ignored build evidence, and commit.

### Task 4: Documentation, desktop evaluation and final review

**Files:** root/effect READMEs, `AGENTS.md`, active notebook/workflow docs, authoring/hardware templates, `docs/sdk-maintenance.md`, `docs/desktop-audition-evaluation.md`, `docs/repository-migration-20260930.md`.

**Interfaces:** executable documented root/compatibility commands and add-effect/update-SDK procedures; no desktop plugin installation.

- [ ] Document corrected SDK boundaries, current candidate behavior, evidence limits, effect registration, manifests, captures, and SDK-update acceptance.
- [ ] Check macOS desktop prerequisites and record wrapper constraints with pinned source links.
- [ ] Run all host/tooling tests, both ARM builds, compatibility commands, structural checks, migration comparison and diff checks. Expected: PASS, with the existing RWX warning reported.
- [ ] Request one independent whole-change review, fix material findings with regression tests, and repeat affected checks.
- [ ] Commit final documentation/results and report actual local state and remaining device/desktop limits.
