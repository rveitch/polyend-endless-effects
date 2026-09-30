# Adding and validating an effect

The repository separates effect code, SDK code and evidence tooling. Add one explicit catalog entry per buildable effect. Device builds link one `Patch::getInstance()` implementation at a time.

1. Create `effects/myEffect/PatchImpl.cpp`, `README.md` and `tests/`. Use a camelCase ID. Include `../../vendor/FxPatchSDK/source/Patch.h`. Implement the official methods, including `Patch::getInstance()`.
2. Start a decision record from [the template](templates/effect-decision-record.md). Describe the reference evidence, intended behavior, normalized controls, action/state transitions and unresolved device checks before assigning authenticity claims.
3. Add an entry to `effects/catalog.json` using repository-relative paths:

```json
{
  "id": "myEffect",
  "name": "My effect",
  "source": "effects/myEffect/PatchImpl.cpp",
  "documentation": "effects/myEffect/README.md",
  "tests": [
    {"source": "effects/myEffect/tests/MyEffectTest.cpp", "linkEffect": true}
  ]
}
```

Set `linkEffect` false only when the test itself includes the implementation, as the existing Juno tests do. Tests must finish and return an error on failure. A catalog entry does not require modifying SDK wrappers or the build engine.

4. Run `make list`, `make test EFFECT=myEffect`, `make test-tools`, then `make build EFFECT=myEffect TOOLCHAIN=/path/to/bin/arm-none-eabi-`. Use `python3 scripts/effects.py test --effect myEffect --sanitize` for optional host undefined-behavior checks.
5. Generate stereo [captures](audio-workflow.md), review controls, transitions, gain, clipping and mono compatibility. Inspect the artifact manifest and record hardware checks using [the hardware template](templates/hardware-validation.md).

## Verified official contract

The pinned SDK exposes three parameters, 48000 Hz stereo float buffers, a singleton patch, a 2400000-float external working buffer, left-foot-switch press/hold actions and a state LED color. Parameter setters are documented as called from the audio thread. The public interface provides no switch release, global cable-routing query or expression-specific callback. The fork's expression conventions and extra name APIs must be checked against the actual official header before use.

Call ordering and control event behavior should be read from `vendor/FxPatchSDK/internal/` when relevant. Do not infer cable configuration from a silent channel. Preserve original dry channels if an effect mixes dry and wet signals.

Device processing must avoid heap allocations and unbounded work. Allocate delay/history memory from the supplied span, initialize it deliberately, and state any remaining local storage. Image plus BSS inspection excludes that external buffer and runtime stack. Host captures allocate memory for finite offline processing.

## Project choices to record

A linear or equal-power mix law is a design choice, not a universal effect requirement. Document knob plateaus, tapers, defaults, gain and bypass behavior. The current Juno diagnostic Mix mapping and provisional wet EQ remain unchanged by this migration. Do not apply a generic control convention retroactively to it.

Separate documented platform facts, measurements, circuit deductions, emulator choices and proposed thresholds. Syntax checks, ARM compilation, signal measurements, listening and device CPU tests answer different questions. Passing one layer must not be reported as passing another.

This workflow adapts the useful organization and evidence practices in [edonahue's authoring notes](https://github.com/edonahue/FxPatchSDK/blob/29761ec0ca83a1f68a6b1d04376ff99db36b0f77/docs/patch-authoring-best-practices.md), circuit conversion notes and audit templates. Effect implementations and fork-only SDK extensions were not imported.
