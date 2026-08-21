from itertools import combinations
import numpy as np
from flsss import gen

rng = np.random.default_rng(1)
nfail = 0

for _ in range(80):
    nrow = int(rng.integers(2, 8))
    ncol = int(rng.integers(1, 6))
    X = rng.integers(
        -8, 9, size=(nrow, ncol),
        dtype=np.int32)
    k = int(rng.integers(1, nrow + 1))
    pick = rng.choice(
        nrow, size=k, replace=False)
    target = X[pick].sum(axis=0)
    slop = int(rng.integers(0, 4))
    lo = target - slop
    hi = target + slop
    if rng.random() < 0.4:
        length = 0
    else:
        length = k
    if ncol == 1 and rng.random() < 0.5:
        Xin = X[:, 0]
        lo_in = int(lo[0])
        hi_in = int(hi[0])
        M = Xin.reshape(-1, 1)
        lo_a = np.array([lo_in])
        hi_a = np.array([hi_in])
    else:
        Xin, lo_in, hi_in = X, lo, hi
        M = X
        lo_a = np.atleast_1d(lo)
        hi_a = np.atleast_1d(hi)
    if length == 0:
        ks = range(1, nrow + 1)
    else:
        ks = (length,)
    want = set()
    for kk in ks:
        for idx in combinations(
                range(nrow), kk):
            ok = True
            for c in range(M.shape[1]):
                s = 0
                for i in idx:
                    s += int(M[i, c])
                if s < int(lo_a[c]) or (
                        s > int(hi_a[c])):
                    ok = False
                    break
            if ok:
                want.add(idx)
    sols = gen(
        Xin, lo_in, hi_in, len=length,
        n_solutions=1 << 20)
    got = set()
    for sol in sols:
        t = []
        for i in sol:
            t.append(int(i))
        t.sort()
        got.add(tuple(t))
    if got != want:
        nfail += 1

for _ in range(30):
    nrow = int(rng.integers(4, 8))
    ncol = 5
    X = rng.integers(
        -8, 9, size=(nrow, ncol),
        dtype=np.int32)
    k = int(rng.integers(1, nrow + 1))
    pick = rng.choice(
        nrow, size=k, replace=False)
    target = X[pick].sum(axis=0)
    slop = int(rng.integers(0, 4))
    lo = target - slop
    hi = target + slop
    if rng.random() < 0.4:
        length = 0
    else:
        length = k
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
                if s < int(lo[c]) or (
                        s > int(hi[c])):
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
        nfail += 1

print("diff nfail=%d %s" % (
    nfail, "FAIL" if nfail else "OK"))
raise SystemExit(1 if nfail else 0)
