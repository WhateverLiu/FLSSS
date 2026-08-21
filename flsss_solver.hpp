#pragma once
// Scalar fixed-cardinality subset-sum branch-and-bound
// (DFS over node arena).
// ncol > 1 uses VerifyBand as a post-filter on
// Contained tuples (other columns after the leading one).
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <limits>
#include <memory>
#include <thread>
#include <utility>
#include <vector>
#include "flsss_common.hpp"
#include "flsss_size.hpp"
#include "trimat.hpp"
#include "node_arena.hpp"
#include "findbound.hpp"

namespace flsss_detail {

[[nodiscard]] inline int resolve_n_threads(int n_threads)
{
    if (n_threads > 0) return n_threads;
    const auto hc = int(std::thread::hardware_concurrency());
    return std::max(1, hc);
}

// Claim up to n units. Returns how many were taken (0 if empty).
[[nodiscard]] inline size_t try_take(std::atomic<size_t>& a, size_t n)
{
    auto left = a.load(std::memory_order_relaxed);
    for (;;) {
        if (left == 0) return 0;
        const auto take = left < n ? left : n;
        if (a.compare_exchange_weak(
                left, left - take,
                std::memory_order_relaxed,
                std::memory_order_relaxed))
            return take;
    }
}

// ----------------------------------------------------------
// Optional post-filter on Contained tuples (ncol > 1).
// ----------------------------------------------------------
struct NoVerify {
    static constexpr bool active = false;
};

template <typename Val>
struct VerifyBounds {
    Val lo;
    Val hi;
};

template <typename Val>
struct VerifyBand {
    static constexpr bool active = true;

    const Val* vv = nullptr;
    size_t ncolRest = 0;
    const VerifyBounds<Val>* bounds = nullptr;
    Val* acc_start = nullptr;
    Val* acc = nullptr;

    VerifyBand() = default;
    VerifyBand(const VerifyBand&) = delete;
    VerifyBand& operator=(const VerifyBand&) = delete;
    ~VerifyBand() { ::delete[] acc_start; }

    void swap(VerifyBand& o) noexcept {
        std::swap(vv, o.vv);
        std::swap(ncolRest, o.ncolRest);
        std::swap(bounds, o.bounds);
        std::swap(acc_start, o.acc_start);
        std::swap(acc, o.acc);
    }

    [[nodiscard]] size_t rest() const { return ncolRest; }

    void bind(const Val* vv_, const VerifyBounds<Val>* bounds_,
              size_t ncol) {
        vv = vv_;
        bounds = bounds_;
        ncolRest = ncol;
    }

    void init(size_t subsetLen) {
        if (acc_start) return;
        const auto r = rest();
        acc_start = ::new Val[(subsetLen + 1) * r];
        std::fill_n(acc_start, r, Val{});
        acc = acc_start + r;
    }

    void copyAccPrefixFrom(const VerifyBand& src, size_t n_slots,
                           size_t subsetLen) {
        const auto r = rest();
        if (r == 0) return;
        init(subsetLen);
        const auto nvals = (n_slots + 1) * r;
        std::copy(src.acc_start, src.acc_start + nvals, acc_start);
        acc = acc_start + nvals;
    }

    [[nodiscard]] size_t current_slot() const {
        return size_t(acc - acc_start) / rest() - 1;
    }

    void acc_append_at(auto sorted_pos) {
        acc_append(vv + size_t(sorted_pos) * rest());
    }

    void acc_append(const Val* src) {
        const auto r = rest();
        const Val* acc_prior = acc - r;
        for (auto k = size_t{0}; k < r; ++k)
            acc[k] = acc_prior[k] + src[k];
        acc += r;
    }

    void acc_popback() {
        const auto r = rest();
        acc = std::max(acc - r, acc_start + r);
    }

    void acc_popback(size_t n_slots) {
        const auto r = rest();
        acc = std::max(
            acc - n_slots * r,
            acc_start + r);
    }

    [[nodiscard]] bool inBand() const {
        const auto r = rest();
        const Val* sums = acc - r;
        for (auto k = size_t{0}; k < r; ++k) {
            if (sums[k] < bounds[k].lo || sums[k] > bounds[k].hi)
                return false;
        }
        return true;
    }
};

struct SharedBudget {
    alignas(64) std::atomic<size_t> sols_got{0};
    alignas(64) std::atomic<size_t> iters_left{0};
    alignas(64) std::atomic<bool> abort{false};
    bool limit_iters = false;
};

template <typename Val, typename Ind, typename Verify = NoVerify>
struct Solver {
    using Arena = NodeArena<Val, Ind>;
    using Hdr   = typename Arena::Hdr;

    // ---- problem data ----
    TriMat<Val, Ind>* M   = nullptr;
    const Val*        v   = nullptr;
    Ind               N   = 0;
    Ind               LEN = 0;
    const Val*        gMin = nullptr;
    const Val*        gMax = nullptr;
    const Ind* rootLB = nullptr;
    const Ind* rootUB = nullptr;

    size_t sizeNeed = 0;
    size_t maxIter  = 0;
    std::chrono::steady_clock::time_point deadline{};
    size_t iterCount = 0;
    size_t iter_quota = 0;
    size_t sol_unreported = 0;
    const Ind* perm = nullptr;

    [[no_unique_address]] Verify verify{};

    Arena       arena;
    size_t curFrame = 0;
    size_t depth    = 0;

    Val scrS{};
    Val scrSR{};

    std::vector<Ind>      hope;
    std::vector<std::vector<Ind>> results;
    std::vector<Ind>      pinned;
    bool                          stop = false;

    std::unique_ptr<SharedBudget> owned_shared;
    SharedBudget* shared = nullptr;

    [[nodiscard]] bool enterNode(size_t F) {
        if (!withinBudget()) return false;

        auto nv  = arena.view(F);
        auto len = nv.hdr->len;
        auto* LB  = nv.lb;
        auto* UB  = nv.ub;

        auto boo =
            findBound(*M, len, *nv.min, *nv.max,
                      LB, *nv.sumLB, UB, *nv.sumUB, scrSR);
        if (boo == BoundResult::Pruned) return false;

        if (boo == BoundResult::Contained) {
            enumerateContainedBox(len, LB, UB);
            return false;
        }

        setUpBranch(nv, len, LB, UB);
        return true;
    }

    void enumerateContainedBox(
        Ind len_in, const Ind* LB, const Ind* UB) {
        const auto len  = size_t(len_in);
        const auto base = hope.size();

        if constexpr (Verify::active) {
            for (auto i = verify.current_slot(); i < base; ++i)
                verify.acc_append_at(hope[i]);
        }

        hope.reserve(base + len);

        auto fillFrom = [&](size_t from) {
            hope.resize(base + from);
            for (auto i = from; i < len; ++i) {
                auto lo = LB[i];
                if (i > 0 && hope[base + i - 1] + 1 > lo)
                    lo = hope[base + i - 1] + 1;
                hope.push_back(lo);
            }
        };

        auto accFrom = [&](auto from) {
            if constexpr (Verify::active) {
                const auto target = base + from;
                const auto slot = verify.current_slot();
                if (slot > target)
                    verify.acc_popback(slot - target);
                for (auto j = from; j < len; ++j)
                    verify.acc_append_at(hope[base + j]);
            }
        };

        fillFrom(0);
        accFrom(0);

        const auto tupleLen = hope.size();
        size_t boxIters = 0;

        auto emitTuple = [&] {
            if (shared && shared->abort.load(
                    std::memory_order_relaxed)) {
                stop = true;
                return;
            }
            std::vector<Ind> dst;
            dst.reserve(tupleLen);
            for (auto k = size_t{0}; k < tupleLen; ++k)
                dst.push_back(perm[hope[k]]);
            results.push_back(std::move(dst));
            if (shared) {
                ++sol_unreported;
                if (sol_unreported >= std::min(
                        kSolChunk, sizeNeed))
                    flushSolCount();
            } else if (results.size() >= sizeNeed) {
                stop = true;
            }
        };

        while (true) {
            if constexpr (Verify::active) {
                if (verify.inBand()) emitTuple();
            } else emitTuple();
            if (stop) break;
            if ((++boxIters & kWallClockCheckMask) == 0) {
                if (shared && shared->abort.load(
                        std::memory_order_relaxed)) {
                    stop = true;
                    break;
                }
                if (std::chrono::steady_clock::now()
                        >= deadline) {
                    if (shared)
                        shared->abort.store(
                            true, std::memory_order_relaxed);
                    stop = true;
                    break;
                }
            }

            auto pos = len;
            auto advanced = false;
            while (pos > 0) {
                --pos;
                if (hope[base + pos] < UB[pos]) {
                    advanced = true;
                    break;
                }
            }
            if (!advanced) break;

            ++hope[base + pos];
            fillFrom(pos + 1);
            accFrom(pos);
        }

        hope.resize(base);
        if constexpr (Verify::active) {
            const auto slot = verify.current_slot();
            if (slot > base) verify.acc_popback(slot - base);
        }
    }

    void pushChild(size_t P, bool lower) {
        const auto pos = arena.hdr(P)->position;
        const auto mid = arena.hdr(P)->mid;
        auto cb = startChild(P);
        if (lower) applyLower(cb, pos, mid);
        else       applyUpper(cb, pos, mid);
        curFrame = cb;
        ++depth;
    }

    void solve() {
        if constexpr (Verify::active)
            verify.init(LEN);
        seedRoot();
        while (depth > 0 && !stop)
            driveOneStage();
    }

    void solveParallel(int n_threads) {
        if constexpr (Verify::active)
            verify.init(LEN);
        owned_shared = std::make_unique<SharedBudget>();
        shared = owned_shared.get();
        shared->limit_iters =
            maxIter != std::numeric_limits<size_t>::max();
        if (shared->limit_iters)
            shared->iters_left.store(
                maxIter, std::memory_order_relaxed);
        seedRoot();
        infra::msfd(*this, n_threads, true);
        flushSolCount();
    }

    void reserveForSearch() {
        auto bis = size_t{1};
        if (size_t(N) > size_t(LEN)) {
            auto g = size_t(N) - size_t(LEN);
            while (g > 1) { g >>= 1; ++bis; }
        }
        const auto arrBytes = mul_or_throw(
            mul_or_throw(size_t{2}, size_t(LEN)), sizeof(Ind));
        const auto chunk = add_or_throw(
            add_or_throw(arena.arraysRel(), arrBytes),
            arena.maxAlign());
        const auto nchunks = add_or_throw(
            mul_or_throw(size_t(LEN), bis + 3), size_t{4});
        arena.reserveBytes(mul_or_throw(nchunks, chunk));
    }

    [[nodiscard]] bool is_solved() const {
        if (shared && shared->abort.load(
                std::memory_order_relaxed))
            return true;
        return depth == 0 || stop;
    }

    void a_step_forward() {
        if (is_solved()) return;
        driveOneStage();
    }

    [[nodiscard]] bool can_split() {
        if (is_solved()) return false;
        return findStealFrame() != Arena::NONE;
    }

    void split(Solver& dest) {
        const auto P = findStealFrame();
        if (P == Arena::NONE) return;
        dest.copySharedFrom(*this);
        dest.reserveForSearch();
        const auto nh = hopeAt(P);
        dest.hope.assign(hope.begin(), hope.begin() + nh);
        if constexpr (Verify::active)
            dest.verify.copyAccPrefixFrom(
                verify, nh, size_t(LEN));
        dest.seedChildFrom(
            *this, P, !arena.hdr(P)->lowerFirst);
        arena.hdr(P)->stage = Stage::AfterB;
    }

    void swap(Solver& o) {
        using std::swap;
        swap(M, o.M);
        swap(v, o.v);
        swap(N, o.N);
        swap(LEN, o.LEN);
        swap(gMin, o.gMin);
        swap(gMax, o.gMax);
        swap(rootLB, o.rootLB);
        swap(rootUB, o.rootUB);
        swap(sizeNeed, o.sizeNeed);
        swap(maxIter, o.maxIter);
        swap(deadline, o.deadline);
        swap(iterCount, o.iterCount);
        swap(iter_quota, o.iter_quota);
        swap(sol_unreported, o.sol_unreported);
        swap(perm, o.perm);
        if constexpr (Verify::active)
            verify.swap(o.verify);
        arena.swap(o.arena);
        swap(curFrame, o.curFrame);
        swap(depth, o.depth);
        swap(scrS, o.scrS);
        swap(scrSR, o.scrSR);
        hope.swap(o.hope);
        results.swap(o.results);
        pinned.swap(o.pinned);
        swap(stop, o.stop);
        owned_shared.swap(o.owned_shared);
        swap(shared, o.shared);
    }

    void merge(Solver& o) {
        o.flushSolCount();
        if (o.results.empty()) return;
        results.reserve(results.size() + o.results.size());
        results.insert(
            results.end(),
            std::make_move_iterator(o.results.begin()),
            std::make_move_iterator(o.results.end()));
        o.results.clear();
    }

private:
    static constexpr size_t kWallClockCheckMask = 4095;
    static constexpr size_t kIterQuotaChunk = 4096;
    static constexpr size_t kSolChunk = 4096;

    void flushSolCount() {
        if (!shared || sol_unreported == 0) return;
        const auto added = sol_unreported;
        sol_unreported = 0;
        const auto n = shared->sols_got.fetch_add(
            added, std::memory_order_relaxed);
        if (n + added >= sizeNeed) {
            shared->abort.store(
                true, std::memory_order_relaxed);
            stop = true;
        }
    }

    [[nodiscard]] bool pastDeadline() const {
        return (iterCount & kWallClockCheckMask) == 0
            && std::chrono::steady_clock::now() >= deadline;
    }

    [[nodiscard]] bool refillIterQuota() {
        if (shared->abort.load(std::memory_order_relaxed)) {
            stop = true;
            return false;
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            shared->abort.store(
                true, std::memory_order_relaxed);
            stop = true;
            return false;
        }
        if (shared->limit_iters) {
            iter_quota = try_take(
                shared->iters_left, kIterQuotaChunk);
            if (iter_quota == 0) {
                shared->abort.store(
                    true, std::memory_order_relaxed);
                stop = true;
                return false;
            }
        } else {
            iter_quota = kIterQuotaChunk;
        }
        return true;
    }

    void driveOneStage() {
        auto F  = curFrame;
        auto st = arena.hdr(F)->stage;

        if (st == Stage::Enter) {
            if (enterNode(F)) {
                arena.hdr(F)->stage = Stage::AfterA;
                pushChild(F, arena.hdr(F)->lowerFirst);
            } else {
                cleanupAndPop(F);
            }
        } else if (st == Stage::AfterA) {
            arena.hdr(F)->stage = Stage::AfterB;
            pushChild(F, !arena.hdr(F)->lowerFirst);
        } else {
            cleanupAndPop(F);
        }
    }

    [[nodiscard]] bool withinBudget() {
        ++iterCount;
        if (shared) {
            if (iter_quota == 0 && !refillIterQuota())
                return false;
            --iter_quota;
            return true;
        }
        if (iterCount > maxIter) {
            stop = true;
            return false;
        }
        if (pastDeadline()) {
            stop = true;
            return false;
        }
        return true;
    }

    void applyLower(size_t cb, Ind pos, Ind mid) {
        const auto cap = arena.hdr(cb)->cap;
        auto* cSumUB = arena.vSumUB(cb);
        auto* dUB    = arena.ub(cb, cap);
        auto c = mid;
        auto i = pos;
        for (; i >= 0; --i, --c) {
            if (dUB[i] <= c) break;
            *cSumUB -= v[dUB[i]];
            dUB[i] = c;
        }
        *cSumUB += (*M)[pos - i - 1][dUB[i + 1]];
    }

    void applyUpper(size_t cb, Ind pos, Ind mid) {
        const auto cap = arena.hdr(cb)->cap;
        auto* cSumLB = arena.vSumLB(cb);
        auto* dLB    = arena.lb(cb);
        auto c = mid + 1;
        auto i = pos;
        for (; i < cap; ++i, ++c) {
            if (dLB[i] >= c) break;
            *cSumLB -= v[dLB[i]];
            dLB[i] = c;
        }
        *cSumLB += (*M)[i - pos - 1][dLB[pos]];
    }

    [[nodiscard]] size_t findStealFrame() {
        auto steal = Arena::NONE;
        for (auto x = curFrame; x != Arena::NONE;
             x = arena.hdr(x)->parentBase) {
            if (arena.hdr(x)->stage == Stage::AfterA)
                steal = x;
        }
        return steal;
    }

    [[nodiscard]] size_t hopeAt(size_t F) {
        auto extra = size_t{0};
        for (auto x = curFrame; x != F && x != Arena::NONE;
             x = arena.hdr(x)->parentBase)
            extra += size_t(arena.hdr(x)->appended);
        return hope.size() - extra;
    }

    void copySharedFrom(Solver& src) {
        M        = src.M;
        v        = src.v;
        N        = src.N;
        LEN      = src.LEN;
        gMin     = src.gMin;
        gMax     = src.gMax;
        rootLB   = src.rootLB;
        rootUB   = src.rootUB;
        sizeNeed = src.sizeNeed;
        maxIter  = src.maxIter;
        deadline = src.deadline;
        perm     = src.perm;
        shared   = src.shared;
        stop     = false;
        iterCount = 0;
        iter_quota = 0;
        sol_unreported = 0;
        depth    = 0;
        curFrame = 0;
        scrS     = {};
        scrSR    = {};
        results.clear();
        hope.clear();
        hope.reserve(size_t(LEN));
        pinned.clear();
        pinned.reserve(size_t(LEN) + 1);
        arena.clear();
        if constexpr (Verify::active) {
            verify.bind(src.verify.vv, src.verify.bounds,
                        src.verify.rest());
            verify.init(size_t(LEN));
        }
    }

    void seedChildFrom(Solver& src, size_t P, bool lower) {
        const auto* ph = src.arena.hdr(P);
        const auto cap = ph->len;
        const auto pos = ph->position;
        const auto mid = ph->mid;
        auto cb = arena.alloc(cap);
        initHdr(cb, Arena::NONE, cap);
        copyBoxFrom(cb, src.arena, P, cap);

        if (lower) applyLower(cb, pos, mid);
        else       applyUpper(cb, pos, mid);

        curFrame = cb;
        depth    = 1;
    }

    struct Branch { Ind position; Ind nz; };

    void setUpBranch(auto& nv, Ind len, Ind* LB, Ind* UB) {
        auto br    = pickBranch(len, LB, UB);
        auto position = br.position;
        auto appended = collapsePinned(br.nz, len, position, LB, UB,
                                      *nv.min, *nv.max,
                                      *nv.sumLB, *nv.sumUB);

        nv.hdr->len        = len;
        nv.hdr->position   = position;
        nv.hdr->mid        = LB[position]
                           + (UB[position] - LB[position]) / 2;
        nv.hdr->appended   = appended;
        nv.hdr->lowerFirst = (position <= len / 2);
    }

    [[nodiscard]] Branch pickBranch(Ind len, const Ind* LB,
                                    const Ind* UB) {
        pinned.clear();
        auto position   = Ind{0};
        auto nonzeroMin = Ind{-1};
        for (auto i = Ind{0}; i < len; ++i) {
            auto gap = UB[i] - LB[i];
            if (gap == 0) {
                pinned.push_back(i);
            } else if (nonzeroMin < 0 || gap < nonzeroMin) {
                nonzeroMin = gap;
                position   = i;
            }
        }
        return { position, Ind(pinned.size()) };
    }

    [[nodiscard]] Ind collapsePinned(
        Ind nz,
        Ind& len,
        Ind& position,
        Ind* LB,
        Ind* UB,
        Val& Min,
        Val& Max,
        Val& sumLB,
        Val& sumUB
    ) {
        if (nz == 0) return 0;

        pinned.push_back(len);
        scrS = {};
        for (auto t = Ind{0}; t < nz; ++t) {
            auto st = pinned[t];
            auto en = pinned[t + 1];
            const auto pos = UB[st];
            hope.push_back(pos);
            if constexpr (Verify::active)
                verify.acc_append_at(pos);
            scrS += v[pos];
            for (auto q = st + 1; q < en; ++q) {
                LB[q - 1 - t] = LB[q];
                UB[q - 1 - t] = UB[q];
            }
        }
        len   -= nz;
        Min   -= scrS;  Max   -= scrS;
        sumLB -= scrS;  sumUB -= scrS;

        auto dec = Ind{0};
        for (auto t = Ind{0};
             t < nz && position > pinned[t]; ++t)
            ++dec;
        position -= dec;
        return nz;
    }

    [[nodiscard]] size_t startChild(size_t P) {
        const auto cap = arena.hdr(P)->len;
        auto cb = arena.alloc(cap);
        initHdr(cb, P, cap);
        copyBoxFrom(cb, arena, P, cap);
        return cb;
    }

    void cleanupAndPop(size_t F) {
        auto* fh = arena.hdr(F);
        if constexpr (Verify::active)
            verify.acc_popback(fh->appended);
        hope.resize(hope.size() - size_t(fh->appended));

        auto parent = fh->parentBase;
        arena.freeTo(F);
        curFrame = parent;
        --depth;
    }

    void seedRoot() {
        auto rb = arena.alloc(LEN);
        initHdr(rb, Arena::NONE, LEN);

        arena.bindSlot(rb, SLOT_MIN, *gMin);
        arena.bindSlot(rb, SLOT_MAX, *gMax);

        auto* rLB = arena.lb(rb);
        auto* rUB = arena.ub(rb, LEN);
        for (auto k = Ind{0}; k < LEN; ++k) {
            rLB[k] = rootLB[k];
            rUB[k] = rootUB[k];
        }

        arena.bindSlot(rb, SLOT_SUM_LB, v[rLB[0]]);
        arena.bindSlot(rb, SLOT_SUM_UB, v[rUB[0]]);
        auto* rSumLB = arena.vSumLB(rb);
        auto* rSumUB = arena.vSumUB(rb);
        for (auto k = Ind{1}; k < LEN; ++k) {
            *rSumLB += v[rLB[k]];
            *rSumUB += v[rUB[k]];
        }

        curFrame = rb;
        depth    = 1;
    }

    void initHdr(size_t fb, size_t parent, Ind cap) {
        auto* h = arena.hdr(fb);
        h->parentBase = parent;
        h->cap        = cap;
        h->len        = cap;
        h->position   = 0;
        h->mid        = 0;
        h->appended   = 0;
        h->stage      = Stage::Enter;
        h->lowerFirst = 0;
    }

    void copyBoxFrom(size_t dst, Arena& srcA, size_t src, Ind cap) {
        arena.bindSlot(dst, SLOT_MIN,    *srcA.vMin(src));
        arena.bindSlot(dst, SLOT_MAX,    *srcA.vMax(src));
        arena.bindSlot(dst, SLOT_SUM_LB, *srcA.vSumLB(src));
        arena.bindSlot(dst, SLOT_SUM_UB, *srcA.vSumUB(src));
        const auto pcap = srcA.hdr(src)->cap;
        auto* sLB = srcA.lb(src);
        auto* sUB = srcA.ub(src, pcap);
        auto* dLB = arena.lb(dst);
        auto* dUB = arena.ub(dst, cap);
        for (auto k = Ind{0}; k < cap; ++k) {
            dLB[k] = sLB[k];
            dUB[k] = sUB[k];
        }
    }
};

template <typename Val, typename Ind, typename Verify = NoVerify>
[[nodiscard]] auto runCoreWithMat(
    TriMat<Val, Ind>& M,
    size_t len,
    const Ind* subset_index_lower_bound,
    const Ind* subset_index_upper_bound,
    size_t N,
    const Ind* v_original_index,
    const Val& lo,
    const Val& hi,
    size_t n_solutions_needed,
    size_t max_iterations,
    std::chrono::steady_clock::time_point deadline,
    const Verify& verify = {},
    int n_threads = 1)
    -> vec<vec<Ind>>
{
    if (n_solutions_needed == 0) return {};

    Solver<Val, Ind, Verify> S;
    if constexpr (Verify::active)
        S.verify.bind(verify.vv, verify.bounds, verify.rest());
    S.M        = &M;
    S.v        = M[0];
    S.N        = Ind(N);
    S.LEN      = Ind(len);
    S.gMin     = &lo;
    S.gMax     = &hi;
    S.rootLB   = subset_index_lower_bound;
    S.rootUB   = subset_index_upper_bound;
    S.sizeNeed = n_solutions_needed;
    S.maxIter  = max_iterations ? max_iterations
        : std::numeric_limits<size_t>::max();
    S.deadline = deadline;
    S.perm     = v_original_index;
    S.pinned.reserve(len + 1);
    S.hope.reserve(len);
    S.results.reserve(result_reserve_cap(n_solutions_needed));
    S.reserveForSearch();

    const auto T = resolve_n_threads(n_threads);
    if (T <= 1) S.solve();
    else S.solveParallel(T);

    if (S.results.size() > n_solutions_needed)
        S.results.resize(n_solutions_needed);

    vec<vec<Ind>> out;
    out.reserve(S.results.size());
    for (auto& r : S.results)
        out.push_back(vec<Ind>(r.begin(), r.end()));
    return out;
}

template <typename Val, typename Ind, typename Verify = NoVerify>
[[nodiscard]] auto runCore(
    size_t len,
    const Ind* subset_index_lower_bound,
    const Ind* subset_index_upper_bound,
    const Val* sorted_v,
    size_t N,
    const Ind* v_original_index,
    const Val& lo,
    const Val& hi,
    size_t n_solutions_needed,
    size_t max_iterations,
    double time_limit_seconds,
    const Verify& verify = {},
    int n_threads = 1)
    -> vec<vec<Ind>>
{
    if (n_solutions_needed == 0) return {};

    TriMat<Val, Ind> M;
    M.build(sorted_v, N, len);

    const auto deadline = deadline_from_seconds(time_limit_seconds);

    return runCoreWithMat<Val, Ind, Verify>(
        M, len,
        subset_index_lower_bound, subset_index_upper_bound,
        N, v_original_index, lo, hi,
        n_solutions_needed, max_iterations, deadline, verify,
        n_threads);
}

}  // namespace flsss_detail
