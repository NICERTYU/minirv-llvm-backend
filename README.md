# MiniRV：真实 LLVM 21 后端与 Verilog RV32I CPU

这个仓库是一个可运行的代码生成练习：独立的 LLVM Target 把手写 LLVM IR 选成少量 RV32I 指令，GNU 汇编器将其编码为机器码，SystemVerilog CPU 在时钟驱动下执行。重点是 LLVM 后端描述、选指、寄存器分配与汇编输出；项目不包含前端。

```text
examples/*.ll
    │ LLVM 21：SelectionDAG → TableGen 选指 → 寄存器分配 → AsmPrinter
    ▼
build/target/minirv-llc → RV32I 汇编
    │ riscv64-unknown-elf-as / objcopy
    ▼
机器码 → rtl/minirv_cpu.sv → 寄存器 a0 中的结果
```

## 环境

- LLVM **21** 开发包：`llvm-config`、`llvm-tblgen`、头文件及共享库
- CMake、Ninja、支持 C++17 的编译器
- RISC-V GNU binutils：`riscv64-unknown-elf-as` 和 `riscv64-unknown-elf-objcopy`
- Icarus Verilog：`iverilog`、`vvp`
- Python 3.8+

macOS/Homebrew 可安装 `llvm`、`cmake`、`ninja`、`icarus-verilog`；RISC-V GNU 工具链需提供上述两个 binutils 命令。构建脚本优先使用 PATH 中的 `llvm-config`，也能识别 Homebrew 的 `/opt/homebrew/opt/llvm/bin/llvm-config`。构建要求 LLVM 主版本为 21。

## 运行

在仓库根目录执行：

```sh
sh scripts/build_target.sh
build/target/minirv-llc -mtriple=riscv32-unknown-elf -O0 \
  examples/arithmetic.ll -o /tmp/minirv-arithmetic.s
cat /tmp/minirv-arithmetic.s
python3 scripts/run_rtl.py examples/arithmetic.ll
python3 -m unittest discover -s tests -v
```

`arithmetic.ll` 计算 `(x + 7) * 4 - y`；仿真默认输入 `x=3, y=5`，应打印 `PASS result=35`。其他示例：

```sh
python3 scripts/run_rtl.py examples/bitmix.ll --arg0 5 --arg1 3 --expected 6
python3 scripts/run_rtl.py examples/return42.ll --expected 42
```

观察 LLVM 选指后的 Machine IR：

```sh
build/target/minirv-llc --verify-machineinstrs --print-after-isel \
  examples/arithmetic.ll -o /tmp/minirv-arithmetic.s 2> /tmp/minirv-isel.txt
cat /tmp/minirv-isel.txt
```

`minirv-llc` 是此仓库链接 LLVM 21 的独立驱动；系统自带的 `llc` 不会自动注册这个外部 Target。当前只输出汇编，编码使用 GNU binutils；还没有 LLVM MCCodeEmitter/ELF 对象输出。

## 后端代码从哪里读

| 文件 | 作用 |
| --- | --- |
| [MiniRVRegisterInfo.td](llvm-target/MiniRV/MiniRVRegisterInfo.td) | 描述 32 个物理寄存器和可分配 GPR 类 |
| [MiniRVInstrInfo.td](llvm-target/MiniRV/MiniRVInstrInfo.td) | 指令的操作数、汇编格式和 SelectionDAG 匹配模式 |
| [MiniRVCodeGen.cpp](llvm-target/MiniRV/MiniRVCodeGen.cpp) | TargetMachine、参数/返回值 lowering、寄存器与栈帧接口 |
| [MiniRVISel.cpp](llvm-target/MiniRV/MiniRVISel.cpp) | 将 TableGen 生成的 DAG selector 接入 LLVM pass |
| [MiniRVAsmPrinter.cpp](llvm-target/MiniRV/MiniRVAsmPrinter.cpp) | MachineInstr 转成 MCInst |
| [MiniRVMC.cpp](llvm-target/MiniRV/MiniRVMC.cpp) | Target/MC 注册与汇编文本打印 |
| [minirv_cpu.sv](rtl/minirv_cpu.sv) | 单周期取指、译码、数据通路与寄存器状态 |
| [run_rtl.py](scripts/run_rtl.py) | 从 IR 到机器码、内存镜像和 RTL 仿真的串联脚本 |

更细的阅读顺序见 [后端导览](docs/llvm-backend-guide.md)。

## 已实现范围

- RV32I 整数寄存器，以及 `add/sub/addi/slli/and/or/xor/ret` 的后端选指和汇编输出。
- C 调用约定下的叶子函数：最多两个 `i32` 参数，返回 `i32` 或 `void`；参数从 `a0/a1` 读取，结果写入 `a0`。
- LLVM 的 SelectionDAG、TableGen 指令匹配、虚拟寄存器与寄存器分配、汇编打印实际参与编译。
- RTL CPU 还译码一部分额外 RV32I 指令（如 load/store、分支与跳转），供后续扩展 Target 使用。测试台把返回地址设为停止哨兵，函数执行 `ret` 后检查 `a0`。

**当前限制：** 后端没有栈帧、寄存器溢出、函数调用、全局变量、LLVM MC 编码器或完整的 RV32I 指令选择。复杂 IR、较大立即数或需要栈空间的函数可能无法编译。RTL 使用固定大小、分离的指令/数据存储器，只作教学仿真；它不是完整的 RISC-V SoC。示例以独立函数为程序入口，测试台直接提供参数。

## 参考

- [LLVM 后端编写指南](https://llvm.org/docs/WritingAnLLVMBackend.html)
- [LLVM 代码生成器](https://llvm.org/docs/CodeGenerator.html)
- [LLVM MIR 文档](https://llvm.org/docs/MIRLangRef.html)
- [RISC-V 非特权 ISA 规范](https://docs.riscv.org/reference/isa/unpriv/unpriv-index.html)
