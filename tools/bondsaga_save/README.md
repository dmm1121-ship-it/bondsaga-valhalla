# Persistence foundation checks

Run the portable C suite from the repository root with Python 3 and a C11 compiler:

```sh
python3 tools/bondsaga_save/run_tests.py --cc=gcc --sanitize
```

The address and undefined-behavior sanitizer option requires compiler/platform
support. To use a portable Zig installation, pass its executable and C frontend
argument separately:

```sh
python tools/bondsaga_save/run_tests.py --cc=/path/to/zig --cc-arg=cc
```

The runner compiles the same container, wire-record, and foundation-validation
sources used by the GBA build. It excludes the physical GBA flash adapter and uses
the test suite's simulated FLASH1M device. Build products and Zig caches default
to the ignored `build/bondsaga-save-tests` directory. No compiler download or
system installation is performed by this script.

`.github/workflows/bondsaga-save.yml` runs the host suite with GCC sanitizers and
builds the approved baseline and candidate Emerald ROM with the same ARM
toolchain. `measure_build.py` produces JSON and Markdown reports from the actual
linker output and ROM file lengths. The workflow also archives per-object
section sizes, ARM compiler stack estimates, build logs, and the candidate map.
The unlinked `layout_probe.c` object reports exact target-ABI sizes of caller-owned
runtime structures; it is never part of the game ROM.

The baseline is pinned in the workflow. Update that reference deliberately when
measuring a later foundation change. Static linker totals are not peak runtime
RAM measurements, object sizes are not automatically retained ROM bytes, and
individual stack-frame estimates are not complete call-chain bounds. Delta and
real flash interruption behavior still require target-device validation.
