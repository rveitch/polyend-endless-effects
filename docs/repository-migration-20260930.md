# Repository migration verification, 2026-09-30

The repository now supports independently selected effects and explicit updates of a pinned official SDK. The approved Juno candidate was relocated without changing DSP or controls. The passthrough implementation was preserved. Historical captures, saved binaries, sibling SDK files and the synced ChatGPT mirror were left intact.

## Scope and identity

Baseline: `20691ca`. Official SDK: `708f08d7c8e365b8a153c66e0e5200fbdaff1ce0` (ABI 11). The live upstream check found no newer official revision on 2026-09-30. The fork was a workflow reference; its effects and extended API were not imported.

The unchanged vendor snapshot is locked by file hashes. Project-owned rules compile its wrapper/entrypoint with one catalog-selected implementation. Compiler dependency scans, per-effect objects, content/configuration invalidation and forced relinking protect selection. Shared headers and included data are hashed; content changes rebuild even when timestamps do not advance. Each new manifest refers to a preserved timestamped ELF beside its .endl. Legacy entry points forward to active source/tests and retain named build overrides.

## Behavioral acceptance

Original Juno SHA-256: `ed8a6735daf91e8cafc8dc26e49eb052a478b9646f525580e9d5b57e279ff180`.
Original passthrough SHA-256: `fe0688366038e82ab4f94af491440ad1852c8019ea08de7d7f5ba40de53943d6`.

A finite common harness rendered the original and relocated Juno for 12 seconds at 48000 Hz with distinct stereo deterministic broadband input, seed 42, Mix 1, Tone/Width 0.5. Modes I, II, I+II and interrupted mode/Tone events were bit-identical, including callback partitions of 64 and 127 samples. Each scenario compared 576000 stereo frames. Existing Mix/vintage tests and passthrough preservation tests also passed.

Raw evidence and WAV hashes are retained locally in ignored `build/migration-baseline/renders-20260930T195104329967/results.json`. The generic verification command is documented in [audio workflow](audio-workflow.md). This proves host relocation parity at the tested settings; it does not certify hardware emulation or ARM/desktop equivalence.

## Tooling acceptance and limits

Catalog/build tests create a third fixture effect and verify selection, generated header dependencies, changed-flag rebuilding, manifests and old entry points. SDK fixtures cover read-only checks, file additions/deletions, modified-snapshot refusal, invalid import/ref and interruption rollback. Independent binary fixtures reject corrupt headers, callback pointers and BSS bounds. Capture tests retain both channels and test sample-scheduled actions across callback partitions, invalid input and metadata mismatches.

Device builds retain the SDK's existing RWX LOAD-segment linker warning. Image/BSS validation covers the pinned 512 KiB linker region, excluding external working memory and runtime stack. Real device timing and CPU budget remain unmeasured. CLion's inspection connector reported the newly created probe file as not found; compilation with warnings as errors is the available code check.

[Desktop evaluation](desktop-audition-evaluation.md) records macOS prerequisites and wrapper changes needed. No desktop package/plugin installation, push, firmware update or pedal deployment occurred. Local commits are the delivery boundary.

## Independent review and final acceptance

The whole change received an independent GPT-6 Astra review. Six important findings were reproduced before fixes: unchanged-timestamp header rebuilding, omitted shared/data dependencies, mutable ELF references, unsupported ABI field widths, colliding comparison outputs and contradictory sidecars. Regression tests now cover those cases. There were no critical findings, deferred minor findings or declined scope decisions. Final tests also cover capture provenance for externally located shared headers/data and separate listening transformations that preserve raw files.

The initial dependency test deliberately advanced beyond Make 3.81 timestamp resolution. The review exposed that this was insufficient for stale-artifact prevention, so actual dependency contents now participate in invalidation. The existing checkout and local-commit delivery boundary follow Ryan's approval; no additional worktree or integration approval pause was needed.

Final verification: 23 tooling/integration tests passed, including all finite effect regressions. Both effects built and passed structural inspection through root and legacy entry points. Undefined-behavior-sanitized host tests passed. End-to-end 12-second capture/comparison/listening-copy commands completed. The final migration rerun remained bit-identical in all four scenarios. Diff and Python syntax checks passed. Local test/build logs are under `build/migration-baseline/`; acceptance captures are under `build/acceptance/`.
