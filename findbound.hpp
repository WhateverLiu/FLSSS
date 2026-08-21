#pragma once

// ==========================================================
// findbound.hpp
//
// Scalar constraint-propagation bound tightening for the
// FLSSS solver. Operates on TriMat<Val, Ind> (row 0 =
// sorted values) plus LB/UB index windows. sumLB / sumUB
// track sum(v[LB[i]]) / sum(v[UB[i]]).
// ==========================================================

#include <cstddef>

#include "trimat.hpp"

namespace flsss_detail {

enum class BoundResult {
    Pruned,
    Branch,
    Contained
};

enum class Tighten {
    Pruned,
    Changed,
    Unchanged
};

// First i in [lo, hi) with row[i] >= val; hi if none.
template <typename Val, typename Ind>
[[nodiscard]] Ind lowerBoundLr(
        const Val* row, Ind lo, Ind hi, const Val& val) {
    for (; lo < hi; ++lo)
        if (row[lo] >= val) break;
    return lo;
}

// Highest i in [lo, hi) with row[i] <= val; lo-1 if none.
template <typename Val, typename Ind>
[[nodiscard]] Ind upperBoundLr(
        const Val* row, Ind lo, Ind hi, const Val& val) {
    auto i = hi - 1;
    for (; i >= lo; --i)
        if (row[i] <= val) break;
    return i;
}

template <typename Val, typename Ind>
[[nodiscard]] bool LBiFind(
        Ind& ciLB, TriMat<Val, Ind>& Mr,
        Ind ci_1LB, Val& SR, Ind I, Ind& J,
        const Ind* UB) {
    if (ciLB < ci_1LB + 1) ciLB = ci_1LB + 1;
    const Val* vv = Mr[0];
    SR += vv[UB[I]];

    while (UB[J] < ciLB - (I - J)) {
        SR -= vv[UB[J]];
        ++J;
    }

    while (true) {
        if (J >= I) {
            if (vv[UB[I]] < SR) return false;
            break;
        }
        if (Mr[I - J][UB[J]] < SR) { SR -= vv[UB[J]]; ++J; }
        else break;
    }

    const auto I_J = I - J;
    ciLB = lowerBoundLr(
        Mr[I_J], Ind(ciLB - I_J),
        Ind(UB[J] + 1), SR) + I_J;
    return true;
}

template <typename Val, typename Ind>
[[nodiscard]] bool UBiFind(
        Ind& ciUB, TriMat<Val, Ind>& Mr,
        Ind ciP1UB, Val& SR, Ind I, Ind& J,
        const Ind* LB) {
    if (ciUB > ciP1UB - 1) ciUB = ciP1UB - 1;
    const Val* vv = Mr[0];
    SR += vv[LB[I]];

    while (LB[J] > ciUB + (J - I)) {
        SR -= vv[LB[J]];
        --J;
    }

    while (true) {
        if (I == J) {
            if (vv[LB[I]] > SR) return false;
            break;
        }
        if (Mr[J - I][LB[J] - (J - I)] > SR) {
            SR -= vv[LB[J]];
            --J;
        }
        else break;
    }

    const auto J_I = J - I;
    ciUB = upperBoundLr(
        Mr[J_I], Ind(LB[J] - J_I),
        Ind(ciUB + 1), SR);
    return true;
}

template <typename Val, typename Ind>
[[nodiscard]] Tighten tightenLowerBounds(
        TriMat<Val, Ind>& Mr, Ind len, const Val& Min,
        Ind* LB, Val& sumLB,
        const Ind* UB, const Val& sumUB, Val& SR) {
    const Val* vv = Mr[0];
    auto I = Ind{0};
    auto J = Ind{0};

    SR = Min; SR += vv[UB[0]]; SR -= sumUB;

    auto before = LB[0];
    LB[0] = lowerBoundLr(vv, LB[0], Ind(UB[0] + 1), SR);
    if (LB[0] > UB[0]) return Tighten::Pruned;
    auto changed = (before != LB[0]);
    sumLB = vv[LB[0]];

    for (I = 1; I < len; ++I) {
        before = LB[I];
        if (!LBiFind(LB[I], Mr, LB[I - 1], SR, I, J, UB))
            return Tighten::Pruned;
        changed |= (before != LB[I]);
        sumLB += vv[LB[I]];
    }
    return changed ? Tighten::Changed : Tighten::Unchanged;
}

template <typename Val, typename Ind>
[[nodiscard]] Tighten tightenUpperBounds(
        TriMat<Val, Ind>& Mr, Ind len, const Val& Max,
        const Ind* LB, const Val& sumLB,
        Ind* UB, Val& sumUB, Val& SR) {
    const Val* vv = Mr[0];
    auto J = Ind(len - Ind{1});
    auto I = J;

    SR = Max; SR += vv[LB[I]]; SR -= sumLB;

    auto before = UB[I];
    UB[I] = upperBoundLr(vv, LB[I], Ind(UB[I] + 1), SR);
    if (LB[I] > UB[I]) return Tighten::Pruned;
    auto changed = (before != UB[I]);
    sumUB = vv[UB[I]];

    for (I = J - 1; I >= 0; --I) {
        before = UB[I];
        if (!UBiFind(UB[I], Mr, UB[I + 1], SR, I, J, LB))
            return Tighten::Pruned;
        changed |= (before != UB[I]);
        sumUB += vv[UB[I]];
    }
    return changed ? Tighten::Changed : Tighten::Unchanged;
}

template <typename Val, typename Ind>
[[nodiscard]] BoundResult findBound(
        TriMat<Val, Ind>& Mr, Ind len,
        const Val& Min, const Val& Max,
        Ind* LB, Val& sumLB,
        Ind* UB, Val& sumUB, Val& SR) {
    auto firstPass = true;

    while (true) {
        auto lo = tightenLowerBounds(
            Mr, len, Min, LB, sumLB, UB, sumUB, SR);
        if (lo == Tighten::Pruned) return BoundResult::Pruned;
        if (!firstPass && lo == Tighten::Unchanged) break;
        firstPass = false;

        auto hi = tightenUpperBounds(
            Mr, len, Max, LB, sumLB, UB, sumUB, SR);
        if (hi == Tighten::Pruned) return BoundResult::Pruned;
        if (hi == Tighten::Unchanged) break;
    }

    if (sumUB < Min || sumLB > Max) return BoundResult::Pruned;
    if (sumLB >= Min && sumUB <= Max) return BoundResult::Contained;
    return BoundResult::Branch;
}

}  // namespace flsss_detail
