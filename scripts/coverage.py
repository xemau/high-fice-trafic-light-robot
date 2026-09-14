#!/usr/bin/env python3
"""Run native tests from clean coverage counters and enforce core coverage gates."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(__file__).resolve().parents[1]
os.chdir(root)
pio = shutil.which("pio") or shutil.which("platformio")
if not pio:
    sys.exit("Install requirements-dev.txt and activate that environment first.")
subprocess.run([pio, "run", "-e", "native", "-t", "clean"], check=True)
subprocess.run([pio, "test", "-e", "native"], check=True)
(root / "coverage").mkdir(exist_ok=True)
gcov = os.environ.get("GCOV", "xcrun llvm-cov gcov" if sys.platform == "darwin" else "gcov")
subprocess.run([
    sys.executable, "-m", "gcovr", "--root", ".",
    "--filter", "src/core/", "--filter", "include/core/",
    "--gcov-executable", gcov,
    "--html-details", "coverage/index.html", "--json-summary", "coverage/summary.json",
    "--xml", "coverage/cobertura.xml", "--print-summary",
    "--fail-under-line", "90", "--fail-under-branch", "90", ".pio/build/native",
], check=True)
