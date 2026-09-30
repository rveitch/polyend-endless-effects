# Updating the official SDK

`vendor/FxPatchSDK/` is imported byte for byte from official Git history. `sdk.lock.json` identifies its repository, full commit and every tracked file hash. Local build customization belongs outside the snapshot. A fresh clone contains the SDK; the sibling checkout is only a reference.

## Check and review

```sh
python3 scripts/sdk.py verify
python3 scripts/sdk.py check --ref master
```

The check stages the requested official ref in ignored temporary build storage, compares file hashes and removes the staging directory. It leaves the committed snapshot and lock unchanged. Use the reported full commit for the later import so a moving branch cannot change the reviewed revision.

Review public `source/Patch.h`, ABI definitions, wrapper, entrypoint, linker, upstream Makefile and license changes. Our build engine intentionally does not execute the upstream Makefile, so upstream compile/link changes need deliberate reconciliation in `buildSupport/arm.mk`. An ABI change raises a compatibility warning. The inspector refuses unsupported formats, even if the new upstream code builds.

As checked on 2026-09-30, official `master` still resolves to `708f08d7c8e365b8a153c66e0e5200fbdaff1ce0`, the imported revision (ABI 11).

## Apply and validate

Save deterministic captures of the current SDK first, using new output names and the same parameters, events and inputs you will use afterward. Preserve the original manifests and artifacts.

```sh
python3 scripts/sdk.py update --ref <reviewedFullCommit> --apply
python3 scripts/sdk.py verify
make test EFFECT=all
make test-tools
make build EFFECT=all TOOLCHAIN=/path/to/bin/arm-none-eabi-
make check EFFECT=all
```

Capture each effect again and compare with the preceding SDK through `scripts/compare.py`. Review any changed samples rather than declaring an SDK update transparent. Record the upstream revision, affected contracts, manifests and remaining hardware checks in a dedicated local update commit.

Updates refuse a snapshot that differs from its lock. Failed fetches or incomplete imports preserve the current snapshot. A pending transaction is rolled back on the next verify/build if the process was interrupted before completion. This recovery protects the import transaction; it is not a multi-process locking scheme. Run SDK imports and builds sequentially in one checkout.

`--source-repo /absolute/path/to/local/git/mirror` is available for offline checks or fixture tests. It fetches committed objects and records the explicit import source. Untracked sibling files are never imported or modified. For ordinary updates, omit it to fetch the official repository.

To abandon an already completed uncommitted update, restore both the vendor directory and lock from the preceding Git revision, including removing files added by that import. Preserve any intentional work first. Do not edit hashes to bless an unexplained vendor modification.
