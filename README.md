# libastromech

Libraries for controlling Star Wars: Galaxy's Edge astromech droids over Bluetooth Low Energy.

This is a monorepo. Each implementation lives in its own top-level directory and is
built, tested and released independently of the others; none depends on another's files.

| Directory | Language | Status |
|-----------|----------|--------|
| [`python/`](python/) | Python >= 3.9 | Available |
| `java/` | Java | Planned |
| `c/` | C | Planned |

Shared, non-code material (images, protocol notes) lives in [`docs/`](docs/).

Each implementation directory has its own README with build and install instructions.

## Build and test

```
make build          # build every implementation present
make test           # verify config sync, then test every implementation
make python-build   # or java-build / c-build, python-test, ...
```

`config/` holds shared droid and personality data. Edit it there, then run
`make sync-config` to copy it into each implementation.
