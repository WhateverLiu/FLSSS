"""Correctness tests for the flsss Python package.

The cases mirror the examples shipped with the original R package
(one-dimensional ``FLSSS`` and multidimensional ``mFLSSSpar``), scaled
down so an independent brute-force enumeration stays cheap. Every
enumeration test asserts that ``gen`` returns *exactly* the set of
qualifying subsets, not merely valid ones.
"""
from __future__ import annotations

import numpy as np
import pytest

from _common import brute_force, canon, gen_all, verify

import flsss


# ---------------------------------------------------------------------------
# One-dimensional Subset Sum  (mirrors FLSSS example I: random numbers)
# ---------------------------------------------------------------------------
def _planted_1d(seed, nrow, subset_size, half_width):
    """Build a 1-D instance with a planted, guaranteed solution."""
    rng = np.random.default_rng(seed)
    v = np.sort(rng.integers(-50, 200, size=nrow).astype(np.int64))
    planted = rng.choice(nrow, size=subset_size, replace=False)
    center = int(v[planted].sum())
    lo, hi = center - half_width, center + half_width
    return v, lo, hi


@pytest.mark.parametrize("seed", range(8))
def test_1d_enumeration_matches_bruteforce(seed):
    v, lo, hi = _planted_1d(seed, nrow=18, subset_size=5, half_width=3)
    got = gen_all(v, lo, hi, length=5)
    assert got == brute_force(v, lo, hi, 5)
    assert len(got) >= 1  # the planted subset must be there


@pytest.mark.parametrize("seed", range(8))
def test_1d_solutions_are_valid(seed):
    v, lo, hi = _planted_1d(seed, nrow=18, subset_size=5, half_width=3)
    sols = flsss.gen(v, lo, hi, len=5, n_solutions=3)
    assert verify(v, lo, hi, sols)


# ---------------------------------------------------------------------------
# Real-world-style set with negatives  (mirrors FLSSS example II)
# ---------------------------------------------------------------------------
REAL_WORLD = np.sort(np.array([
    -1119, -793, -496, -213, 16, 26, 27, 28, 32, 33, 34, 35, 43, 44, 47,
    52, 53, 56, 58, 60, 61, 63, 65, 66, 67, 69, 71, 72, 73, 76, 78, 81,
    82, 85, 86, 87, 90, 93, 98, 310, -441, -548, -645, -149, 305, -248,
], dtype=np.int64))


def test_real_world_fixed_size():
    lo, hi = -5, 5
    got = gen_all(REAL_WORLD, lo, hi, length=6)
    assert got == brute_force(REAL_WORLD, lo, hi, 6)


def test_real_world_variable_size():
    """len == 0: any subset size from 1 to nrow (FLSSS example II variant)."""
    v = REAL_WORLD[:16]
    lo, hi = -2, 2
    got = gen_all(v, lo, hi, length=0)
    assert got == brute_force(v, lo, hi, 0)


# ---------------------------------------------------------------------------
# Smallest subset landing in a band (size swept upward)
# ---------------------------------------------------------------------------
def test_smallest_subset_in_band():
    """Find the least-size subset whose sum lands in [lo, hi]."""
    rng = np.random.default_rng(0)
    values = np.sort(rng.integers(1, 21, size=25).astype(np.int64))
    lo, hi = 95, 100
    found = None
    for k in range(1, len(values) + 1):
        sols = flsss.gen(values, lo, hi, len=k, n_solutions=1)
        if sols:
            found = (k, sols)
            break
    assert found is not None
    k, sols = found
    assert verify(values, lo, hi, sols)
    # No smaller subset can qualify.
    for smaller in range(1, k):
        assert not flsss.gen(values, lo, hi, len=smaller, n_solutions=1)


# ---------------------------------------------------------------------------
# Multidimensional Subset Sum  (mirrors mFLSSSpar example)
# ---------------------------------------------------------------------------
def _planted_md(seed, nrow, ncol, subset_size, half_width):
    rng = np.random.default_rng(seed)
    X = rng.integers(-30, 120, size=(nrow, ncol)).astype(np.int64)
    planted = rng.choice(nrow, size=subset_size, replace=False)
    center = X[planted].sum(axis=0)
    lo = center - half_width
    hi = center + half_width
    return X, lo, hi


@pytest.mark.parametrize("seed", range(6))
def test_md_enumeration_matches_bruteforce(seed):
    X, lo, hi = _planted_md(seed, nrow=16, ncol=3, subset_size=5,
                            half_width=2)
    got = gen_all(X, lo, hi, length=5)
    assert got == brute_force(X, lo, hi, 5)
    assert len(got) >= 1


@pytest.mark.parametrize("seed", range(6))
def test_md_solutions_are_valid(seed):
    X, lo, hi = _planted_md(seed, nrow=20, ncol=5, subset_size=6,
                            half_width=3)
    sols = flsss.gen(X, lo, hi, len=6, n_solutions=2)
    assert verify(X, lo, hi, sols)


def test_md_five_dimensions_exhaustive():
    X, lo, hi = _planted_md(1, nrow=14, ncol=5, subset_size=4,
                            half_width=4)
    got = gen_all(X, lo, hi, length=4)
    assert got == brute_force(X, lo, hi, 4)


# ---------------------------------------------------------------------------
# dtypes, threading, and budgets
# ---------------------------------------------------------------------------
@pytest.mark.parametrize("dtype", [np.int8, np.int16, np.int32, np.int64])
def test_all_signed_dtypes(dtype):
    v = np.arange(1, 9, dtype=dtype)
    got = gen_all(v, 9, 11, length=3)
    assert got == brute_force(v, 9, 11, 3)


def test_threads_give_same_set():
    X, lo, hi = _planted_md(3, nrow=18, ncol=3, subset_size=5,
                            half_width=3)
    serial = canon(flsss.gen(X, lo, hi, len=5,
                             n_solutions=10_000_000, n_threads=1))
    parallel = canon(flsss.gen(X, lo, hi, len=5,
                               n_solutions=10_000_000, n_threads=4))
    assert serial == parallel


def test_n_solutions_cap():
    v = np.arange(1, 9, dtype=np.int64)
    sols = flsss.gen(v, 9, 11, len=3, n_solutions=2)
    assert 0 < len(sols) <= 2
    assert verify(v, 9, 11, sols)


def test_no_solution_returns_empty():
    v = np.arange(1, 9, dtype=np.int64)
    assert flsss.gen(v, 10_000, 10_001, len=2) == []


# ---------------------------------------------------------------------------
# Input validation
# ---------------------------------------------------------------------------
def test_rejects_unsigned_dtype():
    with pytest.raises(TypeError):
        flsss.gen(np.array([1, 2, 3], dtype=np.uint32), 1, 3, len=2)


def test_rejects_bad_bounds_length():
    X = np.ones((5, 2), dtype=np.int32)
    with pytest.raises(ValueError):
        flsss.gen(X, [1], [2], len=2)


def test_scalar_bounds_allowed_for_1d():
    v = np.arange(1, 9, dtype=np.int64)
    sols = flsss.gen(v, 9, 11, len=3)
    assert verify(v, 9, 11, sols)


# ---------------------------------------------------------------------------
# verbose profiling flag
# ---------------------------------------------------------------------------
@pytest.mark.parametrize("n_threads", [1, 4])
@pytest.mark.parametrize("length", [5, 0])
def test_verbose_does_not_change_results(n_threads, length):
    """verbose=True prints a report but must return the same solutions."""
    X, lo, hi = _planted_md(2, nrow=15, ncol=3, subset_size=5,
                            half_width=3)
    quiet = canon(flsss.gen(X, lo, hi, len=length,
                            n_solutions=10_000_000, n_threads=n_threads))
    loud = canon(flsss.gen(X, lo, hi, len=length, verbose=True,
                           n_solutions=10_000_000, n_threads=n_threads))
    assert quiet == loud
