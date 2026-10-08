# 从一个 `add` 看 MiniRV 后端

项目不解析 C。它让 LLVM 21 读取 `examples/*.ll`，随后接入独立的 `MiniRV` Target。可按下面顺序阅读：

1. `MiniRV.td` 定义 Target，包含寄存器和指令描述；CMake 运行 `llvm-tblgen` 生成 `*Gen*.inc`。
2. `MiniRVRegisterInfo.td` 定义 `X0`–`X31` 和可分配的 `GPR`。`MiniRVCodeGen.cpp` 保留 `x0/x1/x2`，并将两个参数映射到 `x10/x11`，返回值映射到 `x10`。
3. `MiniRVInstrInfo.td` 的 `ADDI` 模式把 `(add GPR:$rs1, simm12:$imm)` 匹配为有符号 12 位立即数指令。常数 `7` 满足这个限制；`shl` 的常数移位量匹配 `SLLI`。
4. `MiniRVISel.cpp` 把 TableGen 生成的 `SelectCode` 接入 LLVM 的 SelectionDAG 指令选择 pass。寄存器分配等后续 pass 由 LLVM 运行。
5. `MiniRVAsmPrinter.cpp` 把物理寄存器上的机器指令交给 MC 层；`MiniRVMC.cpp` 使用 TableGen 生成的汇编打印器输出 `addi xN, xM, 7` 等文本。

从 `examples/arithmetic.ll` 开始运行：

```sh
sh scripts/build_target.sh
build/target/minirv-llc --verify-machineinstrs --print-after-isel \
  examples/arithmetic.ll -o /tmp/minirv-arithmetic.s 2> /tmp/minirv-isel.txt
cat /tmp/minirv-isel.txt
cat /tmp/minirv-arithmetic.s
python3 scripts/run_rtl.py examples/arithmetic.ll
```

LLVM 可能重排代数表达式，因此汇编不必逐行对应 IR。重点看选指后的虚拟寄存器如何变成最终的物理寄存器，以及结果如何回到 `a0`。

## 机器码进入电路

`scripts/run_rtl.py` 调用此 Target 输出汇编，再调用 GNU `as` 与 `objcopy` 提取 `.text` 的机器码。脚本把每个 32 位小端指令写为一行十六进制文本。测试台在复位前用 `$readmemh` 装入 `rtl/minirv_cpu.sv` 的指令存储器。

CPU 每个时钟沿执行一条指令：组合逻辑计算 `next_pc`、目的寄存器值和存储器写入；时钟沿更新 PC、寄存器和数据存储器。测试台预设 `x1` 为停止哨兵地址，`ret`（`jalr x0, 0(x1)`）跳转到该地址后记录 `x10`。测试台对结果、非法指令和超时都做检查。

## 可继续实现的后端内容

1. 在 TableGen 中加入 `LW/SW/LUI`，然后实现栈帧和寄存器溢出。
2. 加入比较、条件分支、多个基本块及分支重定位。
3. 实现调用约定和函数调用，管理 caller/callee saved 寄存器。
4. 实现 MCCodeEmitter 与 ELF 对象输出，替换外部 GNU 汇编步骤。

每一步都可以先增加一个 `.ll` 示例，让 `--verify-machineinstrs`、GNU 汇编和 RTL 仿真一起验证。
