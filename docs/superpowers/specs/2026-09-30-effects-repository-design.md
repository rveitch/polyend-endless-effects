# Multiple effects, validation, and upstream SDK maintenance

Date: 2026-09-30
Status: approved by Ryan on 2026-09-30; implementation and verification recorded in ../../repository-migration-20260930.md.

## Intended outcome

Ryan authorized adopting the useful authoring, validation, and comparison workflows
identified in edonahue/FxPatchSDK, and organizing this repository for many different
effects while retaining the ability to update from the official Polyend SDK.

A normal clone must contain everything needed except the compiler. Each effect must
build independently, with repeatable host tests and a clearly identified device
artifact. SDK updates must have a documented provenance and a narrow review boundary.
The existing Juno candidate's audio and controls must survive this organizational
change unchanged. Physical pedal validation remains separate from host verification.

## Verified starting point

- Checkout: `/Users/ryanveitch/nodejs/polyend/polyend-endless-effects`.
- Branch: `fix/chorus-build`; clean at the start of this work.
- Current repository head: `20691ca`, recording listening feedback.
- `example/` owns the live build. The sibling `../FxPatchSDK/` is a reference,
  not a build dependency.
- Copied public header, ABI, wrapper, entrypoint, and linker files are identical
  to official SDK commit `708f08d7c8e365b8a153c66e0e5200fbdaff1ce0`.
- The local Makefile adds overridable build directories and effect selection.
  It does not generate header dependencies.
- Existing finite tests cover Mix, vintage candidate behavior, and passthrough.
  Chorus tests include the implementation directly.
- Chorus source SHA-256 before migration:
  `ed8a6735daf91e8cafc8dc26e49eb052a478b9646f525580e9d5b57e279ff180`.
- Passthrough source SHA-256 before migration:
  `fe0688366038e82ab4f94af491440ad1852c8019ea08de7d7f5ba40de53943d6`.

The earlier code audit and build failure describe historical states. The approved
vintage candidate is the migration baseline, not the duplicated April source.

## SDK organization choice

Use an unmodified, pinned SDK snapshot in `vendor/FxPatchSDK/`, imported from the
official repository. Record its full commit and tracked-file hashes in `sdk.lock.json`.
Keep its license and original build files with the snapshot.

This is preferred to a submodule because clones stay self-contained and updates
appear as ordinary reviewable file changes. A submodule is a viable alternative
but adds initialization and recursive-clone requirements. Depending on the sibling
checkout is rejected because its state is machine-local and independently mutable.

Local build logic belongs outside `vendor/`. Do not patch the vendored Makefile or
wrapper to select effects. Our build compiles the pinned SDK's glue plus exactly one
effect implementation, excluding the upstream example implementation. SDK API and
ABI changes remain distinct from effect organization changes.

## Proposed repository layout

```text
vendor/FxPatchSDK/         unmodified official SDK snapshot
sdk.lock.json             upstream URL, commit, and imported-file hashes
effects/
  catalog.json            explicit buildable effect registry
  junoChorus/
    PatchImpl.cpp         existing candidate, structurally relocated
    tests/                existing Mix and candidate regressions
    README.md             controls, status, build and test entry points
  passthrough/
    PatchImpl.cpp         existing diagnostic
    tests/                sample-preservation regression
    README.md             diagnostic contract and hardware limits
buildSupport/             project-owned ARM build rules
scripts/                  SDK maintenance, artifact inspection, comparison tools
tests/                    tooling regressions and generic capture harness
docs/
  templates/              effect decision record and hardware validation record
  sdk-maintenance.md      check, review, apply, rebuild workflow
  validation.md           project-wide evidence and comparison rules
build/                    ignored per-effect build runs, ELF, .endl, manifests
example/                  compatibility entry points and existing saved artifacts
```

Do not split or redesign the Juno DSP core during this migration. Update include
paths and test locations as necessary, retaining existing behavior and class names.
Extract shared DSP helpers only when multiple effects actually need them.

The catalog records each effect's ID, source, documentation, and host test files.
No directory-wide source glob may link multiple patch singletons into one image.
Adding a future effect means adding its folder, tests, documentation, and registry
entry. The SDK snapshot and project build engine require no per-effect edits.

## Build and compatibility contract

Provide root commands for listing effects, building one or all, running host tests,
and checking artifacts. Use the existing ARM toolchain through a configurable
`TOOLCHAIN` prefix and the host compiler through `HOST_CXX`.

Each effect and build run gets isolated objects and outputs. Compile and link flags
initially match the official SDK and current candidate. Add generated dependency
files so SDK/header edits rebuild affected objects. Preserve the RWX linker warning
as a reported limitation rather than suppressing it.

Retain the documented `make -C example` and diagnostic Makefile entry points as
delegating compatibility targets, including `BUILD_DIR`, `PATCH_NAME`, and toolchain
overrides. Preserve the old finite host-test entry points where practical so the
current CLion configurations and documented workflows continue to resolve. Update
active instructions to root commands; dated capture records retain original paths.

Never clean old saved binaries as part of migration or verification. New verification
uses dedicated build directories. A clean command may remove only its selected
generated build directory, never historical captures or sibling SDK data.

Each device artifact has a JSON manifest with effect ID, source hashes, SDK commit,
compiler version, effective flags, repository revision and dirty-state indication,
artifact/ELF hashes, and artifact paths. A dirty tree is allowed for development;
the manifest must record the actual inputs rather than imply a committed release.

## SDK update workflow

1. Check upstream without modifying the pinned snapshot or current checkout.
2. Resolve a requested official upstream ref to a full commit.
3. Compare imported files and report public API, ABI, wrapper, entrypoint, linker,
   license, and Makefile changes. A change to ABI version requires rebuilding all
   effects and explicitly reporting the compatibility concern.
4. Apply only through an explicit update command after review. Refuse to overwrite
   a snapshot whose files no longer match the lock, and reject incomplete imports.
5. Update the snapshot and lock together, run every host suite and ARM build, inspect
   every artifact, and compare deterministic Juno captures with the preceding SDK.
6. Record the result and remaining hardware checks in a dedicated update commit.

The check command never automatically applies an SDK update. Network failures leave
the lock and snapshot unchanged. Tests exercise update behavior using a temporary
local Git fixture, including additions, deletions, interrupted/invalid imports, and
locally modified vendor files. Do not change the sibling upstream checkout.

## Layered validation

1. Host compilation with warnings as errors and finite existing DSP regressions.
2. Focused tooling tests for effect selection, isolation, dependencies, SDK import,
   manifests, and malformed artifacts.
3. ARM builds for every catalog entry with SDK flags unchanged.
4. Structural inspection: magic, supported ABI, header length, declared image size,
   Thumb entry pointers and their image bounds, and BSS/image RAM bounds against the
   pinned linker region. Optional ABI callbacks remain optional where the SDK permits.
5. Deterministic signal captures and comparative measurements.
6. Manual device listening, stereo routing, control transitions, headroom and CPU
   checks. No host result is described as proof of Juno authenticity or device safety.

Artifact inspection is parameterized by the pinned SDK contract. It must fail
clearly on an unsupported new format instead of silently reusing old offsets.
Report image plus BSS usage separately from the external working buffer. Stack
reports are informational until a real device stack budget is documented.

## Stereo captures and comparison

Provide a finite generic host harness linked to one selected effect. Support sine,
impulse, deterministic broadband, and input WAV stimuli, normalized parameter
settings, action sequences, callback sizes, and configurable duration.

Save both channels in a standard stereo WAV and record source identity, seed,
parameters, actions, sample rate, callback size, SDK revision, effect source hashes,
and capture duration in a JSON sidecar. Use at least 12 seconds for the default
Juno modulation comparison so slow modes span several complete cycles. Allow longer
captures and distinct stereo inputs. Physical recordings remain external evidence.

The comparison tool reports raw L/R and mono-sum levels, peaks, finite/clipped sample
counts, differences, and channel correlation. Mono sum is `(L + R) / 2`, with its
normalization explicitly recorded. Spectral comparisons use matching windows and
do not present spectral similarity as an authenticity score.

Do not align or normalize raw evidence automatically. Optional timing-aligned or
level-matched listening copies are separate outputs with their applied transformations
recorded. Validate sample rates, channel counts, durations, and sidecar consistency;
reject incompatible captures rather than silently truncate them.

The migration acceptance comparison uses identical stimuli and event schedules
before and after relocation, including different callback partitions and all modes.
Any sample difference must be explained before claiming behavior preservation.

## Authoring and documentation

Add an effect decision-record template covering reference evidence, intended fidelity,
controls/tapers, defaults, action and LED behavior, buffer allocation, state changes,
gain staging, CPU assumptions, tests, and unresolved hardware checks.

Document verified SDK behavior separately from project choices. In particular:
expression-on-parameter-2 and parameter-name virtual methods are fork extensions,
not part of the current official public API. Do not import them in this migration.
Keep the candidate's Mix law, provisional wet gain/filtering, mode geometry, and
control plateaus unchanged. Equal-power mixing is not a universal chorus requirement.

Correct the root README's stale compilation-failure claim and update AGENTS.md and
the active notebook links for the new ownership/build boundaries. Retain historical
capture identities and evidence qualifications.

## Desktop audition evaluation

Evaluate the fork's JUCE wrapper separately after the repository migration. First
check local macOS build prerequisites and the wrapper's license/build requirements.
Document findings without installing a plugin or deploying anything to the pedal.

Any later desktop implementation must use a stereo 48 kHz path, queue controls/actions
onto audio processing, preserve the working-buffer contract, and solve or explicitly
enforce the SDK singleton's instance limitation. The fork's non-48-kHz resampler is
not an accepted reference. A successful desktop build does not establish device parity.

## Acceptance and delivery boundaries

- Both existing effects build from the root and compatibility entry points.
- Existing regressions pass and controlled migration captures are sample-identical.
- SDK files match the lock; there is no dependency on the sibling checkout.
- Adding a fixture effect proves independent selection and build isolation.
- Header changes trigger recompilation; artifact corruption fails inspection.
- Stereo comparison outputs retain raw gain/timing and reproducible metadata.
- Saved binaries/captures and unrelated machine-local files are preserved.
- Documentation gives concrete add-effect and SDK-update instructions.
- Local commits are authorized milestones. No push, PR, firmware update, or pedal
  deployment is inferred from this task.

Ryan approved implementation in this chat on 2026-09-30. The implementation plan and migration record describe delivery and verification.
