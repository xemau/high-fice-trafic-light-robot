"""Check the production GPIO adapter against a recording Arduino stub."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class LedOutputTests(unittest.TestCase):
    def test_common_anode_pwm_mapping_startup_gamma_and_failure(self):
        compiler = shutil.which("c++")
        self.assertIsNotNone(compiler, "A host C++ compiler is required")
        stub = ROOT / "test_host" / "arduino_stub"
        with tempfile.TemporaryDirectory(prefix="robot-led-test-") as folder:
            binary = Path(folder) / "led-test"
            subprocess.run([
                compiler, "-std=c++17", "-Wall", "-Wextra", "-Werror",
                "-I", str(stub), "-I", str(ROOT / "include"),
                str(ROOT / "src/hardware/Esp32Adapters.cpp"),
                str(stub / "test_led_outputs.cpp"), "-o", str(binary),
            ], check=True)
            subprocess.run([str(binary)], check=True)
