#!/usr/bin/env python3
"""Cross-check flsss (Python) against the original FLSSS (R).

For each instance we compute the complete qualifying set three ways:

    1. brute force   (independent ground truth, itertools)
    2. flsss.gen     (the Python package under test)
    3. FLSSS / mFLSSSpar via Rscript (the reference implementation)

All three must be identical. Any disagreement is reported as an ALARM and
makes the script exit non-zero. Because we ask each solver to enumerate
*every* solution (huge solution budget, no iteration cap, generous time
limit) on small instances, the qualifying set is well defined and the
comparison is exact, order-independent, and index-base-normalized.

Run:  python3 tests/crosscheck_r_py.py
Requires: Rscript on PATH with the FLSSS package installed.
"""
from __future__ import annotations

import os
import shutil
import subprocess
import sys
import tempfile

import numpy as np

from _common import as_matrix, brute_force, canon, gen_all

HERE = os.path.dirname(os.path.abspath(__file__))
R_RUNNER = os.path.join(HERE, "crosscheck.R")


# --------------------------------------------------------------------------
# Instance generators (shared shapes with test_flsss.py, kept small so the
# brute-force reference stays cheap).
# --------------------------------------------------------------------------
def planted_1d(seed, nrow, subset_size, half_width):
    rng = np.random.default_rng(seed)
    v = np.sort(rng.integers(-50, 200, size=nrow).astype(np.int64))
    planted = rng.choice(nrow, size=subset_size, replace=False)
    center = int(v[planted].sum())
    return v, center - half_width, center + half_width, subset_size


def planted_md(seed, nrow, ncol, subset_size, half_width):
    rng = np.random.default_rng(seed)
    X = rng.integers(-30, 120, size=(nrow, ncol)).astype(np.int64)
    planted = rng.choice(nrow, size=subset_size, replace=False)
    center = X[planted].sum(axis=0)
    return X, center - half_width, center + half_width, subset_size


def cases():
    # (label, X, lo, hi, len)
    for seed in range(6):
        v, lo, hi, k = planted_1d(seed, 18, 5, 3)
        yield f"1d/planted/seed{seed}", v, np.array([lo]), np.array([hi]), k
    # Variable subset size (len == 0).
    v, lo, hi, _ = planted_1d(0, 14, 4, 4)
    yield "1d/varlen/seed0", v, np.array([lo]), np.array([hi]), 0
    for seed in range(6):
        X, lo, hi, k = planted_md(seed, 15, 3, 5, 2)
        yield f"md3/planted/seed{seed}", X, lo, hi, k
    for seed in range(4):
        X, lo, hi, k = planted_md(seed, 13, 5, 4, 3)
        yield f"md5/planted/seed{seed}", X, lo, hi, k
    # Wider bands: many qualifying subsets, to exercise multidimensional
    # enumeration completeness rather than a single planted solution.
    for seed in range(4):
        X, lo, hi, k = planted_md(seed, 16, 2, 5, 30)
        yield f"md2/wide/seed{seed}", X, lo, hi, k
    for seed in range(4):
        X, lo, hi, k = planted_md(seed, 16, 3, 5, 25)
        yield f"md3/wide/seed{seed}", X, lo, hi, k


# --------------------------------------------------------------------------
# R invocation
# --------------------------------------------------------------------------
def run_r(X, lo, hi, length, workdir):
    X = as_matrix(X)
    lo = np.asarray(lo).reshape(-1)
    hi = np.asarray(hi).reshape(-1)
    nrow, ncol = X.shape
    inst = os.path.join(workdir, "inst.txt")
    outp = os.path.join(workdir, "out.txt")
    with open(inst, "w") as f:
        f.write(f"{nrow} {ncol} {length}\n")
        f.write(" ".join(str(int(x)) for x in lo) + "\n")
        f.write(" ".join(str(int(x)) for x in hi) + "\n")
        for r in range(nrow):
            f.write(" ".join(str(int(x)) for x in X[r]) + "\n")
    subprocess.run(
        ["Rscript", R_RUNNER, inst, outp],
        check=True, capture_output=True, text=True)
    sols = []
    with open(outp) as f:
        for line in f:
            line = line.strip()
            if not line or line == "EMPTY":
                continue
            # R indices are 1-based; normalize to 0-based.
            sols.append([int(t) - 1 for t in line.split(",")])
    return canon(sols)


# --------------------------------------------------------------------------
# Driver
# --------------------------------------------------------------------------
def main():
    if shutil.which("Rscript") is None:
        print("ALARM: Rscript not found on PATH; cannot cross-check.")
        return 2

    workdir = tempfile.mkdtemp(prefix="flsss_xcheck_")
    py_alarms = 0   # Python package disagrees with ground truth (serious).
    r_alarms = 0    # R reference disagrees with ground truth.
    n = 0
    try:
        for label, X, lo, hi, length in cases():
            n += 1
            ref = brute_force(X, lo, hi, length)
            py = gen_all(X, lo, hi, length)
            r = run_r(X, lo, hi, length, workdir)

            py_ok = py == ref
            r_ok = r == ref

            status = "OK" if (py_ok and r_ok) else "ALARM"
            if not py_ok:
                py_alarms += 1
            if not r_ok:
                r_alarms += 1
            print(f"[{status}] {label}: "
                  f"ref={len(ref)} py={len(py)} R={len(r)}")
            if not py_ok:
                print("        !! Python != brute force (PACKAGE BUG)")
                _show_diff("py", py, "ref", ref)
            if not r_ok:
                spurious = len(set(r) - set(ref))
                missing = len(set(ref) - set(r))
                print(f"        R != brute force "
                      f"(reference incomplete: missing {missing}, "
                      f"spurious {spurious})")
                _show_diff("R", r, "ref", ref)
    finally:
        shutil.rmtree(workdir, ignore_errors=True)

    print("-" * 60)
    print(f"Instances: {n}")
    print(f"Python vs ground truth : "
          f"{'ALL MATCH' if py_alarms == 0 else f'{py_alarms} MISMATCH'}")
    print(f"R vs ground truth      : "
          f"{'all match' if r_alarms == 0 else f'{r_alarms} mismatch'}")
    if py_alarms:
        print("ALARM: the Python package disagrees with brute force.")
        return 1
    if r_alarms:
        print("ALARM: the R reference (FLSSS) disagrees with brute force, "
              "but the Python package matches it exactly. The Python port "
              "is the trustworthy one here.")
        return 1
    print("All clear: brute force == Python == R on every instance.")
    return 0


def _show_diff(name_a, a, name_b, b):
    sa, sb = set(a), set(b)
    only_a = sorted(sa - sb)[:5]
    only_b = sorted(sb - sa)[:5]
    if only_a:
        print(f"          only in {name_a}: {only_a}")
    if only_b:
        print(f"          only in {name_b}: {only_b}")


if __name__ == "__main__":
    sys.exit(main())
