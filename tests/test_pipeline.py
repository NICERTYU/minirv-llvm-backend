"""End-to-end checks for the LLVM target and the RTL CPU."""

import subprocess
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
COMPILER = ROOT / "build" / "target" / "minirv-llc"


class PipelineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        subprocess.run(["sh", str(ROOT / "scripts" / "build_target.sh")],
                       cwd=ROOT, check=True, capture_output=True, text=True)

    def test_llvm_instruction_selection(self):
        with tempfile.TemporaryDirectory() as directory:
            assembly = Path(directory) / "arithmetic.s"
            result = subprocess.run(
                [str(COMPILER), "--verify-machineinstrs", "--print-after-isel",
                 str(ROOT / "examples" / "arithmetic.ll"), "-o", str(assembly)],
                cwd=ROOT, check=True, capture_output=True, text=True)
            self.assertIn("# After Instruction Selection:", result.stderr)
            self.assertIn("%2:gpr = SLLI", result.stderr)
            self.assertIn("%3:gpr = SUB", result.stderr)
            self.assertIn("$x10 = COPY", result.stderr)
            self.assertIn("ret", assembly.read_text(encoding="utf-8"))

    def test_ir_to_machine_code_to_rtl(self):
        examples = (
            ("arithmetic.ll", 3, 5, 35),
            ("bitmix.ll", 5, 3, 6),
            ("return42.ll", 0, 0, 42),
        )
        for name, arg0, arg1, expected in examples:
            with self.subTest(example=name):
                result = subprocess.run(
                    [sys.executable, str(ROOT / "scripts" / "run_rtl.py"),
                     str(ROOT / "examples" / name), "--arg0", str(arg0),
                     "--arg1", str(arg1), "--expected", str(expected)],
                    cwd=ROOT, check=True, capture_output=True, text=True)
                self.assertIn(f"PASS result={expected}", result.stdout)

    def test_rtl_rejects_wrong_result(self):
        result = subprocess.run(
            [sys.executable, str(ROOT / "scripts" / "run_rtl.py"),
             str(ROOT / "examples" / "return42.ll"), "--expected", "41"],
            cwd=ROOT, capture_output=True, text=True)
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("result=42 expected=41", result.stdout + result.stderr)


if __name__ == "__main__":
    unittest.main()
