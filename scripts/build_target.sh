#!/bin/sh
set -eu

repo_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
if command -v llvm-config >/dev/null 2>&1; then
  llvm_config=$(command -v llvm-config)
elif [ -x /opt/homebrew/opt/llvm/bin/llvm-config ]; then
  llvm_config=/opt/homebrew/opt/llvm/bin/llvm-config
else
  echo "LLVM 21 llvm-config is required" >&2
  exit 1
fi
llvm_cmake=$("$llvm_config" --cmakedir)
cmake -S "$repo_dir/llvm-target/MiniRV" -B "$repo_dir/build/target" \
  -G Ninja -DLLVM_DIR="$llvm_cmake"
cmake --build "$repo_dir/build/target"
