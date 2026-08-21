from itertools import combinations
import numpy as np
from flsss import gen

bad = 0

a = np.array(
    [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8],
    dtype=np.int32)
n = len(a)
want = set()
for idx in combinations(range(n), 3):
    s = 0
    for i in idx:
        s += int(a[i])
    if s >= 10 and s <= 14:
        want.add(idx)


def collect(sols):
    got = set()
    for sol in sols:
        t = []
        for i in sol:
            t.append(int(i))
        t.sort()
        got.add(tuple(t))
    return got


for threads in (1, 2, 4, 0):
    sols = gen(
        a, 10, 14, len=3,
        n_solutions=1 << 20,
        n_threads=threads)
    got = collect(sols)
    if got != want:
        bad += 1
        print("1d T=%s FAIL" % threads)
    else:
        print("1d T=%s OK n=%d" % (
            threads, len(got)))

sols = gen(
    a, 10, 14, len=3,
    n_solutions=1, n_threads=4)
ok = len(sols) == 1
if ok:
    s = 0
    for i in sols[0]:
        s += int(a[int(i)])
    ok = s >= 10 and s <= 14
if not ok:
    bad += 1
print("quota %s" % ("OK" if ok else "FAIL"))

X = np.array(
    [[1, 2], [3, 4], [5, 6]], dtype=np.int64)
want2 = set()
for idx in combinations(range(3), 2):
    s0 = int(X[idx[0], 0] + X[idx[1], 0])
    s1 = int(X[idx[0], 1] + X[idx[1], 1])
    if 4 <= s0 <= 9 and 6 <= s1 <= 10:
        want2.add(idx)
serial = collect(gen(
    X, [4, 6], [9, 10], len=2,
    n_solutions=16, n_threads=1))
par = collect(gen(
    X, [4, 6], [9, 10], len=2,
    n_solutions=16, n_threads=4))
if serial != want2 or par != want2:
    bad += 1
    print("2d FAIL")
else:
    print("2d OK")

serial = collect(gen(
    a, 10, 14, len=0,
    n_solutions=1 << 20, n_threads=1))
par = collect(gen(
    a, 10, 14, len=0,
    n_solutions=1 << 20, n_threads=4))
if serial != par:
    bad += 1
    print("len0 FAIL")
else:
    print("len0 OK n=%d" % len(par))

raise SystemExit(1 if bad else 0)
