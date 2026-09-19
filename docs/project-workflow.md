# Repository and CLion workflow

Verified 2026-09-19.

## Workspace map

| Path under `/Users/ryanveitch/nodejs/polyend` | Role |
| --- | --- |
| `polyend-endless-effects/` | Ryan's Git repository, remote `git@github.com:rveitch/polyend-endless-effects.git` |
| `polyend-endless-effects/example/` | Actual effect build, with copied SDK glue and `source/PatchImpl.cpp` |
| `FxPatchSDK/` | Separate upstream SDK checkout, not an automatic build dependency |
| `agt15-2/` | Existing Arm GNU Toolchain 15.2.Rel1, GCC 15.2.1 |
| `endless/.idea/runConfigurations/` | Old CLion build/clean configurations |
| `.idea/runConfigurations/` | Equivalent configurations added for the larger project |

Effects baseline: `bc31ea43a596f6f19fcffdaf1db41efd5614376e`, April 2, 2026, `Juno Chorus v2`. It was clean at the start of this audit. Upstream SDK baseline: `708f08d7c8e365b8a153c66e0e5200fbdaff1ce0`, March 17, 2026. The SDK already had untracked `.DS_Store` and `.idea/` entries; these were preserved.

The aggregate directory is not a Git repository. Its `AGENTS.md` and CLion files are local workspace configuration and will not be included in a commit made inside the effects repository. Research documentation is stored inside the effects repository so it can be versioned with the code.

## Build

From anywhere on this machine:

```sh
make -C /Users/ryanveitch/nodejs/polyend/polyend-endless-effects/example \
  TOOLCHAIN=/Users/ryanveitch/nodejs/polyend/agt15-2/bin/arm-none-eabi- all
```

For an investigative build that preserves existing binaries, append a new absolute `BUILD_DIR=/tmp/<unique-directory>` argument. The target Makefile uses relative source paths, so the working directory must be `example/`. Supplying only `-f /absolute/Makefile` from the aggregate directory is insufficient.

The build produces `build/patch_<timestamp>.endl` by default. Set `PATCH_NAME=juno_chorus` when a named release is intended. Keep the ELF for size and symbol inspection. The SDK README's `.bin` deployment language is stale; the Makefile and official quick start use `.endl`.

The Makefile compiles with C++20, `-Wall -Wextra -Werror`, Cortex-M7 hard-float options and `-O3`. It does not track header dependencies with generated `.d` files. A fresh build is needed after header changes until that is repaired.

## CLion configurations

The original configurations remain usable in `endless/`. Equivalent `all` and `clean` files were added to the aggregate `.idea/runConfigurations/`, changing paths from `$PROJECT_DIR$/../...` to `$PROJECT_DIR$/...` and explicitly setting the working directory to `polyend-endless-effects/example`.

Both XML files, resolved Makefile/compiler paths and `make -n` invocations passed validation. UI discovery and execution through CLion remain unverified because the connector is unavailable. Reloading the project may be needed for CLion to discover new files. The `all` configuration will still report the real duplicate-definition compilation error until the source is repaired.

`clean` removes the target's `build/` directory. It was inspected using a dry run rather than executed against saved artifacts.

## JetBrains connection

The available connector is named `jetbrains_phpstorm_mcp`, but its saved endpoint still uses an MCP tunnel. Repository and inspection calls returned tunnel errors (429/404). CLion itself was running locally, but a probe of its normal HTTP server did not expose an MCP endpoint. This does not establish that a local MCP connection is impossible; it establishes that this session did not have a working one.

JetBrains Context semantic search also returned HTTP 404 for both child repositories. Re-indexing/enabling those projects would improve the semantic workflow. Targeted filesystem reads and the installed compiler were sufficient for this review, so a tunnel was not needed to complete the research.

## Verification recorded

- Real ARM target build of unchanged effect: **failed**, five duplicate-definition errors at lines 313, 363, 382, 595 and 597. Temporary output directory `/tmp/polyend-chorus-baseline-20260919`.
- Real ARM build of unchanged upstream SDK example: **passed**, emitted `/tmp/polyend-sdk-validation-20260919/patch_20260919_154656.endl`.
- Real ARM build of only the later chorus implementation in a temporary copied target: **passed**. The original source was unchanged. Diagnostic copy: `/var/folders/q4/0k28lb6530z0b7hb927w95hr0000gn/T/polyend-v2-target-v79dctff`, including `build-check.log`. This confirms the compilation repair can be narrow; it does not validate DSP correctness.
- The SDK link emitted an RWX LOAD-segment warning. It did not fail the build and has not been suppressed or changed.
- No pedal deployment, firmware update or audio-hardware validation occurred.

Temporary paths are diagnostic artifacts, not durable release locations. Do not use the successful SDK bitcrusher binary as the Juno effect.
