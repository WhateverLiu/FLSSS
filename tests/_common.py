"""Shared helpers for the flsss test suite.

The tests treat FLSSS as an exact solver: given an integer set and
per-column bounds ``[lo, hi]``, ``gen`` must return *every* qualifying
subset when asked for enough solutions with no time limit. That lets us
compare against an independent brute-force enumeration and, in the
cross-check script, against the original R package.
"""
from __future__ import annotations

import itertools
import os
import sys

import numpy as np

# Allow running straight from a source checkout: prefer an installed
# ``flsss``; otherwise fall back to the locally built extension in
# ``build_local`` produced by ``tests/build_local.sh``.
try:
    import flsss  # noqa: F401
except ModuleNotFoundError:  # pragma: no cover - dev convenience
    _root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    _local = os.path.join(_root, "build_local")
    if os.path.isdir(_local):
        sys.path.insert(0, _local)
    import flsss  # noqa: F401

import flsss  # noqa: E402

# A number large enough to force full enumeration on the small
# instances used by the tests.
ENUMERATE_ALL = 10_000_000


def as_matrix(X):
    """Return X as a 2-D array with shape (nrow, ncol)."""
    X = np.asarray(X)
    if X.ndim == 1:
        return X.reshape(-1, 1)
    return X


def canon(solutions):
    """Canonical form of a solution list: a sorted set of sorted tuples.

    This erases the two degrees of freedom that never carry meaning: the
    order of rows inside one subset, and the order of subsets in the list.
    """
    return sorted(tuple(sorted(int(i) for i in s)) for s in solutions)


def brute_force(X, lo, hi, length):
    """Every qualifying subset, computed by exhaustive enumeration.

    ``length == 0`` means any size from 1 to nrow, matching ``gen``.
    """
    X = as_matrix(X)
    lo = np.asarray(lo).reshape(-1)
    hi = np.asarray(hi).reshape(-1)
    nrow = X.shape[0]
    sizes = [length] if length else range(1, nrow + 1)
    out = []
    for k in sizes:
        for combo in itertools.combinations(range(nrow), k):
            s = X[list(combo)].sum(axis=0)
            if np.all(s >= lo) and np.all(s <= hi):
                out.append(combo)
    return canon(out)


def gen_all(X, lo, hi, length):
    """Full enumeration via flsss.gen (no time or iteration limit)."""
    sols = flsss.gen(
        X, lo, hi, len=length,
        n_solutions=ENUMERATE_ALL, time_limit=0.0)
    return canon(sols)


def verify(X, lo, hi, solutions):
    """True iff every subset's column sums lie within [lo, hi]."""
    X = as_matrix(X)
    lo = np.asarray(lo).reshape(-1)
    hi = np.asarray(hi).reshape(-1)
    for s in solutions:
        idx = [int(i) for i in s]
        col = X[idx].sum(axis=0)
        if not (np.all(col >= lo) and np.all(col <= hi)):
            return False
    return True
