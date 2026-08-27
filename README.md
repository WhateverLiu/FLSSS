# flsss

**flsss** finds subsets of integer rows whose sums fall within given
bounds. It handles both the classic subset-sum problem and its
multidimensional form. A C++20 branch-and-bound solver does the search,
and a small Python API makes it easy to use.

The project is a simplified, accelerated take on
[FLSSS](https://cran.r-project.org/package=FLSSS).

## The problem, gently

Start with a bag of integers. Pick some of them. Their sum is the
*subset sum*. The subset-sum problem asks the reverse question: which
subsets add up to the number you want?

Exact targets are brittle. Real data rarely lands on a single value. So
flsss works with a **band**. You give a lower bound `lo` and an upper
bound `hi`. flsss returns subsets whose sum lands in `[lo, hi]`.
**The wider the band, the easier the search**.

In the multidimensional version, each integer becomes an array of
integers. A subset's sum is an array too. flsss keeps the subsets whose
sum stays inside the band on **every** column.

## Install

```bash
pip install flsss
```

flsss is a compiled extension. Binary wheels ship for common CPython
versions on Linux, macOS, and Windows. If no wheel matches your platform,
`pip` builds from source, and you need a **C++20** compiler (`g++`,
clang, or MSVC).

To compile locally and tune for your CPU:

```bash
FLSSS_NATIVE=1 pip install -e .
```

## Quick start

```python
import numpy as np
from flsss import gen

X = np.array([3, 1, 4, 1, 5, 9, 2, 6], dtype=np.int32)
print(gen(X, lo=10, hi=14, len=3, n_solutions=5, time_limit=3600))
```

Each result is a NumPy array of **0-based row indices**. Index order
inside a solution is not meaningful, and the order of solutions is not
guaranteed. One run above yields five size-3 subsets, for example
`[1, 3, 5]` → values `1, 1, 9` → sum `11`, which sits inside `[10, 14]`.

## Integers only

flsss works on **integers**, not floats. `X` must have a signed integer
dtype: `int8`, `int16`, `int32`, or `int64`. This is deliberate. Integer
arithmetic is exact, so a subset sum is either inside the band or it is
not, with no rounding to argue about. Float input is rejected outright:

```python
import numpy as np
from flsss import gen

X = np.array([1.0, 2.0, 3.0])          # float64
gen(X, lo=1, hi=3, len=2, time_limit=3600)
```

```text
TypeError: X must have a signed integer dtype
```

Have real-valued data? Scale and round it to integers first. Multiply by a power of
ten that keeps the precision you care about, round, then cast:

```python
raw = np.array([1.05, 2.20, 3.14])
X = np.round(raw * 100).astype(np.int64)   # cents, as integers
```

**Remember to scale `lo` and `hi` by the same factor, round and cast**.

## One-dimensional subset sum

Pass a 1-D integer array. Ask for subsets of a given size whose sum lands
in a band. Values may be negative and may repeat.

```python
import numpy as np
from flsss import gen

v = np.array([-7, -3, 1, 2, 5, 8, 11, 14], dtype=np.int64)

# Every size-3 subset summing into [0, 3]:
for s in gen(v, lo=0, hi=3, len=3, n_solutions=100, time_limit=3600):
    idx = sorted(int(i) for i in s)
    print(idx, "->", [int(v[i]) for i in idx], "sum", int(v[idx].sum()))
```

```text
[0, 3, 4] -> [-7, 2, 5]  sum 0
[1, 2, 3] -> [-3, 1, 2]  sum 0
[0, 1, 6] -> [-7, -3, 11] sum 1
[0, 2, 5] -> [-7, 1, 8]  sum 2
[0, 3, 5] -> [-7, 2, 8]  sum 3
[1, 2, 4] -> [-3, 1, 5]  sum 3
```

### Target and error, instead of lo and hi

Often you think in terms of a target `t` with a tolerance `e`. Convert it
in one line:

```python
lo, hi = t - e, t + e
```

The two views are identical. A band of `[lo, hi]` is a target of
`(lo + hi) / 2` with an error of `(hi - lo) / 2`.

### Any subset size

Set `len=0` to drop the size constraint. flsss then searches every size
from 1 to the number of rows.

```python
gen(v, lo=0, hi=3, len=0, n_solutions=100, time_limit=3600)
```

### A needle in a haystack

The classic FLSSS demonstration: hunt for a subset of size 200 hidden
inside a superset of 1000 numbers. The search space is astronomical, yet
flsss finds qualifying subsets in milliseconds.

```python
import numpy as np
from flsss import gen

rng = np.random.default_rng(42)
superset = rng.integers(-10_000, 1_000_000, size=1000).astype(np.int64)

# Plant a size-200 subset so a solution is guaranteed to exist:
planted = rng.choice(1000, size=200, replace=False)
center = int(superset[planted].sum())
lo, hi = center - 100, center + 100       # a narrow band around it

sols = gen(superset, lo, hi, len=200, n_solutions=3, time_limit=3600)
print("found", len(sols), "subsets of size 200")
for s in sols:
    total = int(superset[np.sort(s)].sum())
    print("  sum", total, "in band", lo <= total <= hi)
```

```text
found 3 subsets of size 200
  sum 93816963 in band True
  sum 93816962 in band True
  sum 93816977 in band True
```

### A real-world dataset

The package ships a real-world set of accounting data and asks for
size-10 subsets that land near a target. Here the band is the target
plus or minus 1, a width-3 window. Widen `hi - lo` to admit more slack,
or set `lo == hi` for an exact hit.

```python
import numpy as np
from flsss import gen

superset = np.array([
    -1119924501, -793412295, -496234747, -213654767, 16818148, 26267601,
    26557292, 27340260, 28343800, 32036573, 32847411, 34570996, 34574989,
    43633028, 44003100, 47724096, 51905122, 52691025, 53600924, 56874435,
    58207678, 60225777, 60639161, 60888288, 60890325, 61742932, 63780621,
    63786876, 65167464, 66224357, 67198760, 69366452, 71163068, 72338751,
    72960793, 73197629, 76148392, 77779087, 78308432, 81196763, 82741805,
    85315243, 86446883, 87820032, 89819002, 90604146, 93761290, 97920291,
    98315039, 310120088, -441403864, -548143111, -645883459, -149110919,
    305170449, -248934805, -1108320430, -527806318, -192539936,
    -1005074405, -101557770, -156782742, -285384687, -418917176, 80346546,
    -273215446, -552291568, 86824498, -95392618, -707778486],
    dtype=np.int64)

target = 139254953
sols = gen(superset, lo=target - 1, hi=target + 1, len=10,
           n_solutions=int(1e9), time_limit=3600)
print("solutions found:", len(sols))
idx = np.sort(sols[0])
print("indices", list(map(int, idx)), "sum", int(superset[idx].sum()))
```

```text
solutions found: 342
indices [23, 30, 32, 34, 35, 41, 48, 49, 54, 59] sum 139254952
```

Large sums need headroom. Here the running totals exceed a 32-bit range,
so the data is `int64`. Pick a dtype that holds your values *and* their
cumulative sums.

**A note on threads.** With more than one thread, flsss is
nondeterministic in *order*, never in *membership*. Rerun the same call
and any of these may change:

- **Solution order.** The list comes back in a different sequence.
- **The first hit.** `sols[0]` differs, so any "pick one" line varies.
- **The sample under a cap.** When `n_solutions` is smaller than the
  total, you get a different subset of the valid solutions.
- **The count under a limit.** Under `time_limit` or `max_iterations`,
  threads race, so how many solutions arrive before the cutoff varies.
- **The verbose report.** Per-thread timings shift from run to run.

Set `n_threads=1` for a fully reproducible run. Enumerate everything with
no budget, and the complete solution *set* is identical every time,
whatever the thread count.

## Multidimensional subset sum

Pass a 2-D integer array, shape `(nrow, ncol)`. Give `lo` and `hi` as
vectors of length `ncol`. flsss keeps subsets whose **column** sums land
in the band on every column at once.

```python
import numpy as np
from flsss import gen

X = np.array([[4, 1],
              [1, 5],
              [2, 2],
              [3, 4],
              [5, 1],
              [1, 1]], dtype=np.int64)

# Size-3 subsets with both column sums in [6, 8]:
for s in gen(X, lo=[6, 6], hi=[8, 8], len=3, n_solutions=100, time_limit=3600):
    idx = sorted(int(i) for i in s)
    print(idx, "colsum", [int(c) for c in X[idx].sum(0)])
```

```text
[0, 1, 5] colsum [6, 7]
[1, 4, 5] colsum [7, 7]
[0, 1, 2] colsum [7, 8]
[1, 2, 4] colsum [8, 8]
[2, 3, 5] colsum [6, 7]
[0, 3, 5] colsum [8, 6]
```

### A planted-solution pattern

This pattern seeds a known subset, then asks flsss to recover it. It is
the backbone of the test suite.

It is not a real-world workflow. With many dimensions, a random band rarely
holds any qualifying subset unless it is very wide. A narrow band on random targets
usually returns nothing. So we borrow a real subset's column sums as the
target. That guarantees at least one solution, which makes the example
runnable and the test verifiable.

```python
import numpy as np
from flsss import gen

rng = np.random.default_rng(0)
X = rng.integers(-30, 120, size=(20, 5)).astype(np.int64)

planted = rng.choice(20, size=6, replace=False)
center = X[planted].sum(axis=0)
lo, hi = center - 3, center + 3          # a tight band around the truth

sols = gen(X, lo, hi, len=6, n_solutions=5, time_limit=3600)
for s in sols:
    assert np.all(X[np.sort(s)].sum(0) >= lo)
    assert np.all(X[np.sort(s)].sum(0) <= hi)
```

## The `gen` API

```text
gen(X, lo, hi, len=0, n_solutions=1,
    max_iterations=0, time_limit=0.0, n_threads=0,
    verbose=False)
```

| argument | meaning |
| --- | --- |
| `X` | signed integer array, shape `(nrow, ncol)` or `(nrow,)` |
| `lo`, `hi` | per-column bounds, length `ncol`; a scalar is allowed when `ncol == 1` |
| `len` | subset size; `0` means any size from 1 to `nrow` |
| `n_solutions` | maximum number of solutions to return |
| `max_iterations` | node budget; `0` means unlimited |
| `time_limit` | wall-clock limit in seconds; `0` means none |
| `n_threads` | workers; default `0` uses all hardware concurrency; `1` is serial |
| `verbose` | if `True`, print a findBound() timing report to stderr before returning |

`X` must have a **signed** integer dtype: `int8`, `int16`, `int32`, or
`int64`. Pick the smallest type that holds your data and its running sums;
smaller types run faster. `gen` returns a list of NumPy arrays of row
indices.

### Controlling the search

- **Count.** `n_solutions` caps the output. Set it high to enumerate
  everything; the solver stops early once the cap is met.
- **Budget.** `max_iterations` limits the number of search nodes.
- **Time.** `time_limit` stops the search after a wall-clock deadline.
- **Threads.** `n_threads` splits the work. The default `0` uses all
  hardware concurrency; pass `1` for serial. Results are the same set
  regardless of thread count.
- **Profiling.** `verbose=True` prints a findBound() timing report to
  stderr before returning, without changing the result.

Under a budget or a deadline, flsss returns what it found so far. Every
returned subset is always valid; you may simply get fewer of them.

## Correctness

flsss is tested as an **exact** solver. On small instances, asking for
enough solutions with no time limit must return *every* qualifying subset
and nothing else. The suite checks this against an independent brute-force
enumeration, across dtypes, dimensions, negatives, variable sizes, and
thread counts.

```bash
# build the extension in-tree, then run the tests
bash tests/build_local.sh
python -m pytest tests/test_flsss.py -q
```

### Cross-check against the R package

`tests/crosscheck_r_py.py` feeds identical integer instances to both
implementations and to brute force, then compares the complete solution
sets. It **alarms** on any disagreement.

```bash
python tests/crosscheck_r_py.py   # needs Rscript with FLSSS installed
```

On every instance tested, `flsss.gen` matches brute force exactly. The R
reference matches on the multidimensional path. On the one-dimensional,
fixed-length path, the R package (FLSSS 9.2.8) silently **drops** valid subsets on some
integer instances (most often those whose sum lands on a band edge). The
cause is floating point. R carries the bounds and running sums as
doubles, so a value that is exactly on the edge can round just outside
it and get pruned. flsss sidesteps this entirely: it keeps everything in
exact integer arithmetic, so an edge sum is unambiguously in or out. R
never returns wrong subsets; it returns too few. The Python port does not
share this gap.
