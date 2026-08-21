"""Python API for FLSSS_gen."""
import os
import sys

# Python 3.8+ does not search PATH for extension
# DLL dependencies. MinGW builds need libstdc++,
# libgcc, and winpthread from the compiler bin dir.
if sys.platform == "win32":
    import shutil
    from pathlib import Path

    _gxx = shutil.which("g++") or shutil.which("gcc")
    if _gxx:
        os.add_dll_directory(
            str(Path(_gxx).resolve().parent))

import numpy as np

from . import _core

__all__ = ["gen", "FLSSS_gen"]


def gen(
    X,
    lo,
    hi,
    len=0,
    n_solutions=1,
    max_iterations=0,
    time_limit=0.0,
    n_threads=1,
):
    """Find row subsets whose column sums lie in [lo, hi].

    Parameters
    ----------
    X : array_like
        Signed integer matrix, shape (nrow, ncol) or (nrow,).
    lo, hi : array_like
        Per-column target bounds, length ncol. A scalar is
        allowed when ncol == 1.
    len : int
        Subset size. 0 means any size from 1 to nrow.
    n_solutions : int
        Maximum number of solutions to return.
    max_iterations : int
        Node budget. 0 means unlimited.
    time_limit : float
        Wall-clock limit in seconds. 0 means none.
    n_threads : int
        Worker threads. 1 is serial. 0 uses hardware concurrency.

    Returns
    -------
    list of ndarray
        Each array is a vector of row indices.
    """
    X = np.ascontiguousarray(X)
    if X.ndim == 0:
        raise ValueError("X must be 1-D or 2-D")
    if not np.issubdtype(X.dtype, np.signedinteger):
        raise TypeError(
            "X must have a signed integer dtype")
    ncol = 1 if X.ndim == 1 else X.shape[1]
    lo = np.ascontiguousarray(lo, dtype=X.dtype)
    hi = np.ascontiguousarray(hi, dtype=X.dtype)
    if lo.ndim == 0:
        lo = lo.reshape(1)
    if hi.ndim == 0:
        hi = hi.reshape(1)
    if lo.shape != (ncol,) or hi.shape != (ncol,):
        raise ValueError(
            "lo and hi must have length ncol")
    return _core.gen(
        X, lo, hi, int(len), int(n_solutions),
        int(max_iterations), float(time_limit),
        int(n_threads))


FLSSS_gen = gen
