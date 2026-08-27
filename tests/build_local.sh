#!/usr/bin/env bash
# Build the flsss extension in-tree for running the test suite without a
# pip install. Output lands in ../build_local/flsss so tests/_common.py
# can import it directly.
#
# On macOS the shipped setup.py splits the four value-type instantiations
# into separate translation units; each emits parlay's
# `static inline thread_local worker_info`, and macOS `ld` has no
# --allow-multiple-definition, so the link fails with a duplicate symbol.
# For local testing we compile all instantiations in a single TU instead.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
root="$(cd "$here/.." && pwd)"
out="$root/build_local"
mkdir -p "$out/flsss"

py="${PYTHON:-python3}"
# Capture four separate lines (include dirs + ext suffix). A single
# `read` would only consume the first line, leaving the rest empty.
{
  read -r PYINC
  read -r PBINC
  read -r NPINC
  read -r EXT
} < <("$py" - <<'PYEOF'
import pybind11, numpy, sysconfig
print(sysconfig.get_path('include'))
print(pybind11.get_include())
print(numpy.get_include())
print(sysconfig.get_config_var('EXT_SUFFIX'))
PYEOF
)

CXX="${CXX:-clang++}"
arch_flag=""
if [ "$(uname -s)" = "Darwin" ]; then
  arch_flag="-arch $(uname -m)"
fi

"$CXX" -O3 -ffast-math -pthread -std=c++20 $arch_flag \
  -shared -undefined dynamic_lookup \
  -I"$root" -I"$root/python/flsss" \
  -I"$PYINC" -I"$PBINC" -I"$NPINC" \
  "$root/python/flsss/_core.cpp" "$here/_core_all_inst.cpp" \
  -o "$out/flsss/_core$EXT"

cp "$root/python/flsss/__init__.py" "$out/flsss/"
echo "Built $out/flsss/_core$EXT"
