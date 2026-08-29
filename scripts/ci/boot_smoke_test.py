#!/usr/bin/env python3
# Headless Renode boot smoke test: loads the ELF, runs for a fixed time, and checks that
# boot reached os_app_main without landing in a fault handler -- a regression check for
# bad MPU/privilege setup. NOTE: covers boot + first-task-start only; see TODO for planned tick/queue assertions.
import argparse
import re
import subprocess
import sys
import tempfile
from pathlib import Path

FAULT_SYMBOL = "Default_Handler"
REACHED_MAIN_SYMBOL = "g_reached_main"
REACHED_MAIN_EXPECTED = 0xA5A5A5A5

RESC_TEMPLATE = """\
using sysbus

mach create "rtos"
machine LoadPlatformDescription @{platform}
sysbus LoadELF @{elf}
emulation RunFor "{run_for}"
echo "reached_main:"
sysbus ReadDoubleWord {reached_main_addr}
echo "pc:"
cpu PC
quit
"""


def nm_symbol_address(nm, elf, symbol):
    out = subprocess.run([nm, elf], capture_output=True, text=True, check=True).stdout
    for line in out.splitlines():
        parts = line.split()
        if len(parts) == 3 and parts[2] == symbol:
            return int(parts[0], 16)
    raise SystemExit(f"symbol '{symbol}' not found in {elf} (nm: {nm})")


def run_renode(renode, resc_path):
    proc = subprocess.run(
        [renode, "--disable-gui", "--console", str(resc_path)],
        capture_output=True, text=True, timeout=120,
    )
    return proc.stdout + proc.stderr


def parse_hex_after_label(log, label):
    m = re.search(re.escape(label) + r"\s*\n\s*(0x[0-9A-Fa-f]+)", log)
    if not m:
        return None
    return int(m.group(1), 16)


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--elf", required=True)
    ap.add_argument("--platform", required=True, help="Renode .repl platform description")
    ap.add_argument("--run-for", default="00:00:01", help="Renode 'emulation RunFor' duration")
    ap.add_argument("--renode", default="renode")
    ap.add_argument("--nm", default="arm-none-eabi-nm")
    args = ap.parse_args()

    elf = Path(args.elf).resolve()
    platform = Path(args.platform).resolve()
    if not elf.exists():
        raise SystemExit(f"ELF not found: {elf}")
    if not platform.exists():
        raise SystemExit(f"platform description not found: {platform}")

    reached_main_addr = nm_symbol_address(args.nm, str(elf), REACHED_MAIN_SYMBOL)
    fault_addr = nm_symbol_address(args.nm, str(elf), FAULT_SYMBOL)

    resc = RESC_TEMPLATE.format(
        platform=platform,
        elf=elf,
        run_for=args.run_for,
        reached_main_addr=hex(reached_main_addr),
    )

    with tempfile.NamedTemporaryFile("w", suffix=".resc", delete=False) as f:
        f.write(resc)
        resc_path = Path(f.name)

    try:
        log = run_renode(args.renode, resc_path)
    finally:
        resc_path.unlink(missing_ok=True)

    reached_main = parse_hex_after_label(log, "reached_main:")
    pc = parse_hex_after_label(log, "pc:")

    print(f"g_reached_main = {reached_main:#010x}" if reached_main is not None else "g_reached_main = <not found>")
    print(f"pc              = {pc:#010x}" if pc is not None else "pc              = <not found>")

    failures = []
    if reached_main != REACHED_MAIN_EXPECTED:
        got = f"{reached_main:#010x}" if reached_main is not None else "<not found>"
        failures.append(
            f"g_reached_main == {got}, "
            f"expected {REACHED_MAIN_EXPECTED:#010x} -- boot never reached os_app_main"
        )
    if pc is not None and pc == fault_addr:
        failures.append(
            f"PC == Default_Handler ({fault_addr:#010x}) -- CPU is stuck in a fault handler "
            "(HardFault/MemManage/BusFault/UsageFault)"
        )

    if failures:
        print("\nFAIL:")
        for msg in failures:
            print(f"  - {msg}")
        print("\n--- Renode log ---")
        print(log)
        sys.exit(1)

    print("\nPASS: boot reached os_app_main, no fault detected")


if __name__ == "__main__":
    main()
