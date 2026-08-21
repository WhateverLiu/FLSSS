import numpy as np
from flsss import gen

rng = np.random.default_rng(1)
nrow, ncol, k = 50, 5, 35
X = rng.integers(
    -20, 21, size=(nrow, ncol),
    dtype=np.int32)

pick = rng.choice(
    nrow, size=k, replace=False)
pick.sort()
target = np.zeros(ncol, dtype=np.int32)
for i in pick:
    for c in range(ncol):
        target[c] += int(X[i, c])

lo = np.zeros(ncol, dtype=np.int32)
hi = np.zeros(ncol, dtype=np.int32)
for c in range(ncol):
    lo[c] = target[c] - int(
        rng.integers(0, 1))
    hi[c] = target[c] + int(
        rng.integers(0, 1))

planted = []
for i in pick:
    planted.append(int(i))
planted = tuple(planted)

sols = gen(
    X, lo, hi, 
    len=0,
    # n_solutions=1 << 20,
    n_solutions=1
)

bad = 0
got = set()
for sol in sols:
    t = []
    for i in sol:
        t.append(int(i))
    t.sort()
    got.add(tuple(t))
    # if len(t) != k:
    #     bad += 1
    #     continue
    for c in range(ncol):
        s = 0
        for i in t:
            s += int(X[i, c])
        if s < int(lo[c]) or s > int(hi[c]):
            bad += 1
            break

if planted not in got:
    bad += 1

print("dim5 30x5 planted=%s" % (
    "OK" if planted in got else "FAIL"))
print("dim5 nsol=%d invalid=%d %s" % (
    len(got), bad,
    "FAIL" if bad else "OK"))
raise SystemExit(1 if bad else 0)
