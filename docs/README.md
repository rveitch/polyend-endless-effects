# Juno chorus project notebook

Research and local verification date: **2026-09-19**. This notebook records evidence and a proposed completion path. It does not declare the emulation finished or hardware-validated.

## Start here

1. [Current repository workflow](../README.md): effect selection, builds and tests. [Earlier project workflow](project-workflow.md) retains dated CLion and build-repair evidence.
2. [Code audit](code-audit.md): confirmed defects and DSP assumptions in the committed source.
3. [Polyend platform contract](platform/polyend-endless.md): current SDK, manual, controls, routing and update findings.
4. [Juno circuit and measurement evidence](research/juno-chorus-evidence.md): what the sources actually establish.
5. [Reference implementations](research/reference-implementations.md): Junologue, Hera, TAL-related source and their differing goals.
6. [Historical decisions and corrections](research/history-and-decisions.md): Ryan's preferences versus previous assistant claims.
7. [Completion plan](completion-plan.md): staged deliverables and acceptance criteria.
8. [Validation protocol](validation.md): objective DSP checks and controlled listening comparisons.

9. [Stereo captures, 2026-09-28](stereo-captures-20260928.md): recorded mode comparisons and unresolved bypass headroom concern.

## Current authoring and maintenance

- [Effect authoring](effect-authoring.md) and [decision/hardware templates](templates/effect-decision-record.md)
- [SDK maintenance](sdk-maintenance.md)
- [Stereo capture and comparison](audio-workflow.md)
- [Migration verification](repository-migration-20260930.md)
- [Desktop VST3 build and REAPER setup](../desktop/README.md)
- [Desktop audition evaluation](desktop-audition-evaluation.md)

## Latest capture and design milestones

- [Slow TAL-inspired blue revision](blue-tal-20261002.md)
- [Accepted gain-corrected pedal baseline](pedal-gain47-20261002.md)

- [Implemented vintage candidate and pedal testing handoff](vintage-candidate-20260929.md)

- [Mode I reference comparison](modeI-reference-comparison-20260929.md)
- [Mode II and I+II reference comparison](other-modes-reference-comparison-20260929.md)
- [DSP revision proposal, including vintage wet coloration](dsp-revision-proposal-20260929.md)

## Current assessment

The approved vintage candidate is implemented and host-tested on `fix/chorus-build`.
Its original fast combined mode is now superseded by the user-approved slow
TAL-inspired blue extension; red and green and the provisional wet filtering remain intact. See the candidate handoff above for
its exact build, validation limits and next hardware tests. The earlier audit
records historical defects and is not a description of the current source.

## Evidence rules

- **Observed:** reproduced from the current local checkout or a cited recording/measurement.
- **Documented:** stated by the SDK, manufacturer manual, circuit document or implementation author.
- **Inferred:** derived from those sources and identified as an inference.
- **Proposed:** a design choice, threshold or test target for this project.
- **Unknown:** requires better documentation, a device test or a reference capture.

Exact Juno delay/rate measurements describe particular hardware and measurement methods. They are useful calibration anchors, not universal component tolerances. Plugin behavior is a separate reference from circuit behavior.

Update the relevant document when a measurement or decision changes. Include date, commit, reference version, signal levels and device routing so later sessions can reproduce it. Do not carry forward old assistant assertions as facts.
