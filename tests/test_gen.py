from itertools import combinations
import numpy as np
from flsss import FLSSS_gen, gen

bad = 0

a = np.array(
    [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8],
    dtype=np.int32)
lo, hi = 10, 14
n = len(a)

want = set()
for k in range(1, n + 1):
    for idx in combinations(range(n), k):
        s = 0
        for i in idx:
            s += int(a[i])
        if s >= lo and s <= hi:
            want.add(idx)

sols = gen(
    a, lo, hi, len=0, n_solutions=1 << 20)
got = set()
for sol in sols:
    t = []
    for i in sol:
        t.append(int(i))
    t.sort()
    got.add(tuple(t))

if got != want:
    bad += 1
print("len0 got=%d want=%d %s" % (
    len(got), len(want),
    "FAIL" if got != want else "OK"))

if FLSSS_gen is not gen:
    bad += 1
    print("alias FAIL")
else:
    print("alias OK")

X = np.array(
    [100, 1, 1, 1, 1, 1, 1, 1, 1],
    dtype=np.int32)
n = len(X)
want = set()
for k in range(1, n + 1):
    for idx in combinations(range(n), k):
        s = 0
        for i in idx:
            s += int(X[i])
        if s == 100:
            want.add(idx)
sols = gen(
    X, 100, 100, len=0,
    n_solutions=1 << 20)
got = set()
for sol in sols:
    t = []
    for i in sol:
        t.append(int(i))
    t.sort()
    got.add(tuple(t))
if (0,) not in got or got != want:
    bad += 1
    print("singleton FAIL")
else:
    print("singleton OK")

raise SystemExit(1 if bad else 0)
