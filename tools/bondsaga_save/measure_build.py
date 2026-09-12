#!/usr/bin/env python3
"""Report measured linker-region and padded ROM deltas from two complete builds."""

import argparse
import json
from pathlib import Path
import re
import subprocess


def read_regions(path):
    text = path.read_text(encoding="utf-8", errors="replace")
    regions = {}
    for name, used, unit in re.findall(
            r"^\s*(EWRAM|IWRAM|ROM):\s+(\d+)\s+(B|KB|MB)\b", text, re.MULTILINE):
        regions[name] = int(used) * {"B": 1, "KB": 1024, "MB": 1048576}[unit]
    if set(regions) != {"EWRAM", "IWRAM", "ROM"}:
        raise ValueError("Missing complete linker memory report: {}".format(path))
    return regions


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--baseline-log", type=Path, required=True)
    parser.add_argument("--current-log", type=Path, required=True)
    parser.add_argument("--baseline-rom", type=Path, required=True)
    parser.add_argument("--current-rom", type=Path, required=True)
    parser.add_argument("--baseline-ref", required=True)
    parser.add_argument("--current-ref", required=True)
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--size-tool", default="arm-none-eabi-size")
    parser.add_argument("--object", action="append", default=[], type=Path)
    args = parser.parse_args()
    baseline = read_regions(args.baseline_log)
    current = read_regions(args.current_log)
    baseline["padded_gba_bytes"] = args.baseline_rom.stat().st_size
    current["padded_gba_bytes"] = args.current_rom.stat().st_size
    delta = {name: current[name] - baseline[name] for name in baseline}
    object_sections = {}
    for path in args.object:
        object_sections[str(path)] = subprocess.check_output(
            [args.size_tool, "-A", str(path)], text=True)

    report = {"baseline_ref": args.baseline_ref, "current_ref": args.current_ref,
              "baseline_bytes": baseline, "current_bytes": current,
              "delta_bytes": delta, "compiled_object_sections": object_sections,
              "limits_bytes": {"ROM": 33554432, "EWRAM": 262144, "IWRAM": 32768}}
    args.output_dir.mkdir(parents=True, exist_ok=True)
    (args.output_dir / "measurements.json").write_text(
        json.dumps(report, indent=2) + "\n", encoding="utf-8")
    rows = ["## Bondsaga save-foundation build measurements", "",
            "Baseline: `{}`. Candidate: `{}`.".format(args.baseline_ref, args.current_ref),
            "", "| Measure | Baseline bytes | Candidate bytes | Change bytes |",
            "|---|---:|---:|---:|"]
    for name in baseline:
        rows.append("| {} | {} | {} | {:+} |".format(
            name, baseline[name], current[name], delta[name]))
    rows += ["", "ROM is linker-used bytes; padded_gba_bytes is the exported file size.",
             "Linker RAM totals cover static allocations, not peak stack or heap demand.",
             "Unreferenced foundation code may be discarded by the normal ROM link;",
             "compiled object sections below show its cost before that removal.", ""]
    for name, output in object_sections.items():
        rows += ["### " + name, "", "```text", output.rstrip(), "```", ""]
    (args.output_dir / "measurements.md").write_text("\n".join(rows), encoding="utf-8")
    print("\n".join(rows))


if __name__ == "__main__":
    main()
