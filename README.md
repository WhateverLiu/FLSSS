# flsss

Python bindings for **FLSSS**, a branch-and-bound solver for
(multidimensional) subset-sum: find row subsets whose column
sums lie in given bounds.

```python
import numpy as np
from flsss import gen

X = np.array([3, 1, 4, 1, 5, 9, 2, 6], dtype=np.int32)
print(gen(X, lo=10, hi=14, len=3, n_solutions=5))
```

## Install

```bash
pip install flsss
```

This is a compiled extension. Binary wheels are built for
common CPython versions on Linux, macOS, and Windows. If no
wheel matches your platform, `pip` builds from source and you
need a **C++20** compiler (`g++`, clang, or MSVC).

For a local compile tuned to this CPU:

```bash
FLSSS_NATIVE=1 pip install -e .
```

## `gen`

```text
gen(X, lo, hi, len=0, n_solutions=1,
    max_iterations=0, time_limit=0.0, n_threads=1)
```

| argument | meaning |
| --- | --- |
| `X` | signed integer array, shape `(nrow, ncol)` or `(nrow,)` |
| `lo`, `hi` | per-column target bounds (length `ncol`) |
| `len` | subset size; `0` means any size from 1 to `nrow` |
| `n_solutions` | maximum number of solutions to return |
| `max_iterations` | node budget; `0` means unlimited |
| `time_limit` | wall-clock limit in seconds; `0` means none |
| `n_threads` | workers; `1` is serial; `0` uses hardware concurrency |

Returns a list of NumPy arrays of row indices.

Related R package: [FLSSS on CRAN](https://CRAN.R-project.org/package=FLSSS).
