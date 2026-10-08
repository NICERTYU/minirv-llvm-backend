"""Compile LLVM IR and run its RISC-V machine code on the Verilog CPU."""

import argparse
import shutil
import subprocess
import tempfile
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def find_tool(name, fallback=None):
    found = shutil.which(name)
    if found:
        return found
    if fallback and Path(fallback).exists():
        return fallback
    raise SystemExit("missing tool: " + name)


def run(command):
    subprocess.run(command, check=True)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path, help="LLVM IR file")
    parser.add_argument("--compiler", help="LLVM code generator; defaults to build/target/minirv-llc")
    parser.add_argument("--arg0", type=int, default=3)
    parser.add_argument("--arg1", type=int, default=5)
    parser.add_argument("--expected", type=int, default=35)
    args = parser.parse_args()
    compiler = args.compiler or str(ROOT / "build" / "target" / "minirv-llc")
    if not Path(compiler).is_file():
        raise SystemExit("missing MiniRV compiler; run sh scripts/build_target.sh first")
    assembler = find_tool("riscv64-unknown-elf-as")
    objcopy = find_tool("riscv64-unknown-elf-objcopy")
    iverilog = find_tool("iverilog")
    vvp = find_tool("vvp")

    with tempfile.TemporaryDirectory() as directory:
        tmp = Path(directory)
        asm_file = tmp / "program.s"
        obj_file = tmp / "program.o"
        bin_file = tmp / "program.bin"
        hex_file = tmp / "program.hex"
        sim_file = tmp / "sim.vvp"
        run([compiler, "-mtriple=riscv32-unknown-elf", "-O0",
             str(args.input), "-o", str(asm_file)])
        run([assembler, "-march=rv32i", "-mabi=ilp32",
             "-o", str(obj_file), str(asm_file)])
        run([objcopy, "-O", "binary", "-j", ".text", str(obj_file), str(bin_file)])
        data = bin_file.read_bytes()
        if len(data) % 4:
            raise SystemExit("text size is not a multiple of four bytes")
        if len(data) > 256 * 4:
            raise SystemExit("program exceeds the 256-word instruction memory")
        words = [
            "{:08x}".format(int.from_bytes(data[i:i + 4], "little"))
            for i in range(0, len(data), 4)
        ]
        words.extend(["00000013"] * (256 - len(words)))
        hex_file.write_text(
            "\n".join(words) + "\n", encoding="utf-8"
        )
        run([iverilog, "-g2012", "-s", "tb_minirv", "-o", str(sim_file),
             str(ROOT / "rtl" / "minirv_cpu.sv"),
             str(ROOT / "rtl" / "tb_minirv.sv")])
        run([vvp, str(sim_file), "+hex=" + str(hex_file),
             "+arg0=" + str(args.arg0), "+arg1=" + str(args.arg1),
             "+expected=" + str(args.expected)])


if __name__ == "__main__":
    main()
