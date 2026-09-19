# Juno chorus project notebook

Research and local verification date: **2026-09-19**. This notebook records evidence and a proposed completion path. It does not declare the emulation finished or hardware-validated.

## Start here

1. [Project and build workflow](project-workflow.md): repository ownership, CLion run configurations, compiler and known build failure.
2. [Code audit](code-audit.md): confirmed defects and DSP assumptions in the committed source.
3. [Polyend platform contract](platform/polyend-endless.md): current SDK, manual, controls, routing and update findings.
4. [Juno circuit and measurement evidence](research/juno-chorus-evidence.md): what the sources actually establish.
5. [Reference implementations](research/reference-implementations.md): Junologue, Hera, TAL-related source and their differing goals.
6. [Historical decisions and corrections](research/history-and-decisions.md): Ryan's preferences versus previous assistant claims.
7. [Completion plan](completion-plan.md): staged deliverables and acceptance criteria.
8. [Validation protocol](validation.md): objective DSP checks and controlled listening comparisons.

## Current assessment

The project has a usable SDK and compiler, but the committed effect cannot build because `PatchImpl.cpp` includes two full implementations. The later revision is a musical experiment with a dual-rate I+II mode, not a calibrated model of the measured Juno-60 both-button behavior.

The recommended direction is a measured Juno-60 baseline, with any lush parallel I+II variant clearly treated as an extension. This is a recommendation awaiting Ryan's target preference, not a retroactive change to his requirements.

## Evidence rules

- **Observed:** reproduced from the current local checkout or a cited recording/measurement.
- **Documented:** stated by the SDK, manufacturer manual, circuit document or implementation author.
- **Inferred:** derived from those sources and identified as an inference.
- **Proposed:** a design choice, threshold or test target for this project.
- **Unknown:** requires better documentation, a device test or a reference capture.

Exact Juno delay/rate measurements describe particular hardware and measurement methods. They are useful calibration anchors, not universal component tolerances. Plugin behavior is a separate reference from circuit behavior.

Update the relevant document when a measurement or decision changes. Include date, commit, reference version, signal levels and device routing so later sessions can reproduce it. Do not carry forward old assistant assertions as facts.
