#!/usr/bin/env python3
"""Compile and run the portable save-foundation tests, without an emulator."""

import argparse
import os
from pathlib import Path
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cc", default=os.environ.get("CC", "cc"),
                        help="C compiler executable (not a shell command)")
    parser.add_argument("--cc-arg", action="append", default=[],
                        help="compiler prefix argument; use --cc-arg=cc for Zig")
    parser.add_argument("--sanitize", action="store_true",
                        help="enable address and undefined-behavior sanitizers")
    parser.add_argument("--build-dir", type=Path,
                        default=Path("build/bondsaga-save-tests"))
    args = parser.parse_args()

    root = Path(__file__).resolve().parents[2]
    output_dir = args.build_dir.resolve()
    output_dir.mkdir(parents=True, exist_ok=True)
    executable = output_dir / ("bondsaga_save_test.exe" if os.name == "nt"
                               else "bondsaga_save_test")
    command = [args.cc, *args.cc_arg, "-std=c11", "-Wall", "-Wextra",
               "-Werror", "-pedantic", "-O1", "-g", "-iquote", str(root / "include")]
    if args.sanitize:
        command += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
    command += [str(root / "src/bondsaga_save.c"),
                str(root / "src/bondsaga_data.c"),
                str(root / "src/bondsaga_foundation.c"),
                str(root / "tests/bondsaga_save_test.c"),
                "-o", str(executable)]
    environment = dict(os.environ)
    environment.setdefault("ZIG_GLOBAL_CACHE_DIR", str(output_dir / "zig-global-cache"))
    environment.setdefault("ZIG_LOCAL_CACHE_DIR", str(output_dir / "zig-local-cache"))
    print("Compiling portable Bondsaga persistence tests", flush=True)
    subprocess.run(command, cwd=root, env=environment, check=True)
    if args.sanitize:
        environment.setdefault("ASAN_OPTIONS", "detect_leaks=1:halt_on_error=1")
        environment.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")
    subprocess.run([str(executable)], cwd=root, env=environment, check=True)
    return 0


if __name__ == "__main__":
    try:
        sys.exit(main())
    except (OSError, subprocess.CalledProcessError) as error:
        print("Save-foundation tests failed: {}".format(error), file=sys.stderr)
        sys.exit(1)
