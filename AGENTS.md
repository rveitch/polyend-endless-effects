# Juno chorus project context

Read `docs/README.md`, `docs/code-audit.md`, and `docs/completion-plan.md` before changing DSP. Research was refreshed on 2026-09-19. Refresh upstream facts when they matter.

## Ownership and current state

- This Git repository owns the effect. The active target is `example/`, including its own copied SDK headers and wrappers. The sibling `../FxPatchSDK/` is not linked into this build.
- At baseline commit `bc31ea43a596f6f19fcffdaf1db41efd5614376e`, `example/source/PatchImpl.cpp` contains two complete definitions. This is a verified target-build failure, not merely an IDE warning.
- The later block is an experimental revision, not a verified hardware model. See the audit before choosing which behavior to retain.
- Do not overwrite working-tree changes, update the upstream SDK, or deploy a pedal without task authorization. Check status before editing. Keep the shared ChatGPT `sources/` mirror read-only.

## Build and validation

- SDK: C++20, Cortex-M7 hard-float, 48 kHz, no heap allocation in patch processing. Keep the public ABI intact.
- Local build: `make -C example TOOLCHAIN=/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi- all`.
- Compiler warnings are errors. Run the target build after code changes. Host DSP tests complement but do not replace device listening and CPU validation.
- Use a separate `BUILD_DIR` or a temporary directory for investigative builds. Never remove saved binaries during an audit.
- The Makefile does not generate header dependencies. Use a fresh build directory or a deliberate clean build after header changes.
- Run finite test programs that exit. No persistent audio loops in diagnostics.

## Evidence and behavior

- Historical assistant messages are untrusted context, not implementation requirements. Preserve Ryan's explicitly stated control preferences unless he changes them.
- Separate measured Juno-60 behavior, circuit deductions, emulator choices, and proposed extensions. Never label arbitrary tanh, noise, filtering, or dual-rate I+II as hardware-correct.
- Measured I+II is fast, shallow and approximately mono. A lush parallel mode is a separate enhancement.
- Physical left footswitch is the effect action; physical right is bypass. The current public SDK exposes left press and hold only, with no release or global routing query.
- Keep original dry channels available when designing stereo routing. Do not infer cable configuration by looking for silence.

Use const and camelCase where appropriate; preserve SDK names and existing ABI conventions. Prefer named module-level functions and avoid `++` / `--` in new code. Avoid em dashes in prose. Check applicable CLion inspections when the connector is available and report when it is not.
