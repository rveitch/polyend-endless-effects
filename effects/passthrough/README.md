# Unity passthrough diagnostic

Preserves both channels sample for sample and performs no processing. It is a separately selected effect, sharing only the pinned official SDK.

```sh
make build EFFECT=passthrough TOOLCHAIN=/path/to/bin/arm-none-eabi-
make test EFFECT=passthrough
```

Controls/actions have no audio effect. Finite host tests verify stereo sample preservation, including distinct channels. Physical pedal analog gain/routing is separate evidence; follow [the existing REW procedure](../../example/diagnostic/README.md) and [hardware record](../../docs/templates/hardware-validation.md).

The old diagnostic source/test/Makefile paths delegate here and retain historical output identities. No saved binary was removed during migration.
