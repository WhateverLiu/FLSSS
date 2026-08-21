from itertools import combinations
import numpy as np
from flsss import gen

bad = 0

a = np.array(
    [3, 1, 4, 1, 5, 9, 2, 6, 5, 3, 5, 8],
    dtype=np.int32)
n = len(a)

lo, hi, k = 10, 14, 3
want = set()
for idx in combinations(range(n), k):
    s = 0
    for i in idx:
        s += int(a[i])
    if s >= lo and s <= hi:
        want.add(idx)
sols = gen(
    a, lo, hi, len=k, n_solutions=1 << 20)
got = set()
for sol in sols:
    t = []
    for i in sol:
        t.append(int(i))
    t.sort()
    got.add(tuple(t))
if got != want:
    bad += 1
print("len3 got=%d want=%d %s" % (
    len(got), len(want),
    "FAIL" if got != want else "OK"))

lo, hi, k = 20, 40, 8
want = set()
for idx in combinations(range(n), k):
    s = 0
    for i in idx:
        s += int(a[i])
    if s >= lo and s <= hi:
        want.add(idx)
sols = gen(
    a, lo, hi, len=k, n_solutions=1 << 20)
got = set()
for sol in sols:
    t = []
    for i in sol:
        t.append(int(i))
    t.sort()
    got.add(tuple(t))
if got != want:
    bad += 1
print("len8 got=%d want=%d %s" % (
    len(got), len(want),
    "FAIL" if got != want else "OK"))

total = 0
for x in a:
    total += int(x)
sols = gen(
    a, total, total, len=n, n_solutions=4)
ok = len(sols) == 1
if ok:
    idx = []
    for i in sols[0]:
        idx.append(int(i))
    idx.sort()
    ok = idx == list(range(n))
if not ok:
    bad += 1
print("full_set %s" % ("OK" if ok else "FAIL"))

raise SystemExit(1 if bad else 0)
