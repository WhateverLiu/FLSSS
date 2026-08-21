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

for dt in (
        np.int8, np.int16,
        np.int32, np.int64):
    X = a.astype(dt)
    sols = gen(
        X, 10, 14, len=3,
        n_solutions=1 << 20)
    got = set()
    for sol in sols:
        t = []
        for i in sol:
            t.append(int(i))
        t.sort()
        got.add(tuple(t))
    if got != want:
        bad += 1
        print("dtype %s FAIL" % dt)
        break
else:
    print("dtypes OK")

ok = False
try:
    gen(a.astype(np.uint32), 10, 14, len=3)
except TypeError:
    ok = True
if not ok:
    bad += 1
print("unsigned %s" % ("OK" if ok else "FAIL"))

ok = False
try:
    gen(a.astype(np.float64), 10, 14, len=3)
except TypeError:
    ok = True
if not ok:
    bad += 1
print("float %s" % ("OK" if ok else "FAIL"))

ok = False
try:
    gen(np.zeros((2, 2, 2), dtype=np.int32),
        [0, 0], [1, 1], len=1)
except ValueError:
    ok = True
if not ok:
    bad += 1
print("3d %s" % ("OK" if ok else "FAIL"))

ok = False
try:
    gen(np.array([[1, 2], [3, 4]],
                 dtype=np.int32),
        [1], [2, 3], len=1)
except ValueError:
    ok = True
if not ok:
    bad += 1
print("lo_hi %s" % ("OK" if ok else "FAIL"))

sols = gen(a, 10, 14, len=3, n_solutions=0)
ok = sols == []
if not ok:
    bad += 1
print("nsol0 %s" % ("OK" if ok else "FAIL"))

sols = gen(a, 10, 14, len=3, n_solutions=1)
ok = len(sols) == 1
if ok:
    s = 0
    for i in sols[0]:
        s += int(a[int(i)])
    ok = s >= 10 and s <= 14
if not ok:
    bad += 1
print("nsol1 %s" % ("OK" if ok else "FAIL"))

sols = gen(
    np.array([], dtype=np.int32), 0, 0, len=0)
ok = sols == []
if not ok:
    bad += 1
print("empty %s" % ("OK" if ok else "FAIL"))

X = np.array(
    [[1, 2], [3, 4], [5, 6]], dtype=np.int64)
want = set()
for idx in combinations(range(3), 2):
    s0 = int(X[idx[0], 0] + X[idx[1], 0])
    s1 = int(X[idx[0], 1] + X[idx[1], 1])
    if 4 <= s0 <= 9 and 6 <= s1 <= 10:
        want.add(idx)
sols = gen(
    X, [4, 6], [9, 10], len=2, n_solutions=16)
got = set()
for sol in sols:
    t = []
    for i in sol:
        t.append(int(i))
    t.sort()
    got.add(tuple(t))
if got != want:
    bad += 1
print("2d got=%d want=%d %s" % (
    len(got), len(want),
    "FAIL" if got != want else "OK"))

X = np.array(
    [1, 0, 2, 1, 0,
     0, 3, 1, 0, 2,
     2, 1, 0, 2, 1,
     1, 1, 1, 1, 1,
     0, 2, 0, 3, 0,
     3, 0, 1, 0, 2],
    dtype=np.int32)
X = X.reshape(6, 5)
nrow, ncol = 6, 5
lo = [3, 1, 2, 3, 1]
hi = [6, 4, 5, 6, 4]
for length in (0, 3, 4):
    if length == 0:
        ks = range(1, nrow + 1)
    else:
        ks = (length,)
    want = set()
    for kk in ks:
        for idx in combinations(
                range(nrow), kk):
            ok = True
            for c in range(ncol):
                s = 0
                for i in idx:
                    s += int(X[i, c])
                if s < lo[c] or s > hi[c]:
                    ok = False
                    break
            if ok:
                want.add(idx)
    sols = gen(
        X, lo, hi, len=length,
        n_solutions=1 << 20)
    got = set()
    for sol in sols:
        t = []
        for i in sol:
            t.append(int(i))
        t.sort()
        got.add(tuple(t))
    if got != want:
        bad += 1
    print("dim5 len=%d got=%d want=%d %s" % (
        length, len(got), len(want),
        "FAIL" if got != want else "OK"))

ok = False
try:
    gen(np.full(20, 100, dtype=np.int8),
        0, 1, len=1)
except RuntimeError:
    ok = True
if not ok:
    bad += 1
print("int8 overflow %s" % (
    "OK" if ok else "FAIL"))

raise SystemExit(1 if bad else 0)
