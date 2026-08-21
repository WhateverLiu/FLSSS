#pragma once
#include "flsss_column_heuristic.hpp"
#include "flsss_common.hpp"
#include "flsss_solver.hpp"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>


// Input mutation: does not modify X. Overwrites
// targetSumLowerBound[0..ncol) and
// targetSumUpperBound[0..ncol) in place.
template <typename Val>
[[nodiscard]] bool tighten_bounds_for_len(
    const Val* X, size_t nrow, size_t ncol, size_t len,
    Val* targetSumLowerBound,
    Val* targetSumUpperBound)
{
    auto col = vec<Val>::uninitialized(nrow);
    for (size_t c = 0; c < ncol; ++c) {
        for (size_t r = 0; r < nrow; ++r)
            col[r] = X[r * ncol + c];
        Val loSum{};
        Val hiSum{};
        if (len == nrow) [[unlikely]] {
            loSum = hiSum =
                std::accumulate(col.begin(), col.end(), Val{});
        } else {
            std::nth_element(
                col.begin(), col.begin() + len, col.end());
            loSum = std::accumulate(
                col.begin(), col.begin() + len, Val{});
            std::nth_element(
                col.begin(), col.end() - len, col.end());
            hiSum = std::accumulate(
                col.end() - len, col.end(), Val{});
        }
        targetSumLowerBound[c] =
            std::max(targetSumLowerBound[c], loSum);
        targetSumUpperBound[c] =
            std::min(targetSumUpperBound[c], hiSum);
        if (targetSumLowerBound[c] > targetSumUpperBound[c])
            return false;
    }
    return true;
}


// Input mutation: does not modify X or any caller-owned buffers.
template <typename Val>
[[nodiscard]] vec<Val> column_totals(
    const Val* X, size_t nrow, size_t ncol)
{
    vec<Val> totals(ncol);
    for (size_t r = 0; r < nrow; ++r) {
        const auto* row = X + r * ncol;
        for (size_t c = 0; c < ncol; ++c)
            totals[c] += row[c];
    }
    return totals;
}

// Input mutation: does not modify col_totals.
// Overwrites lo[0..ncol) and hi[0..ncol) in place.
template <typename Val>
void complement_bounds_in_place(
    const vec<Val>& col_totals,
    Val* lo, Val* hi, size_t ncol)
{
    for (size_t c = 0; c < ncol; ++c) {
        const auto origLo = lo[c];
        const auto origHi = hi[c];
        lo[c] = col_totals[c] - origHi;
        hi[c] = col_totals[c] - origLo;
    }
}

// Input mutation: does not modify nrow or subset.
// Avoid the name `small`: Windows headers define it
// as a macro for `char`.
template <typename Ind>
[[nodiscard]] vec<Ind> complement_indices(
    size_t nrow, const vec<Ind>& subset)
{
    vec<uint8_t> in(nrow, 0);
    for (const auto i : subset)
        in[size_t(i)] = 1;
    vec<Ind> out;
    out.reserve(nrow - subset.size());
    for (size_t i = 0; i < nrow; ++i)
        if (!in[i]) out.push_back(Ind(i));
    return out;
}

// Assumes bounds are already tightened.
template <typename Val, typename Ind, size_t Ncol = 0>
[[nodiscard]] vec<vec<Ind>> FLSSS_nonzero_len_with_leading(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    size_t leadingC,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    std::optional<
        std::chrono::steady_clock::time_point> deadline = {},
    int n_threads = 1)
{
    const auto nc = flsss_detail::ncol_or<Ncol>(ncol);
    if (nc == 1)
        leadingC = 0;
    else if (leadingC >= nc)
        return {};

    vec<Val> leading_col;
    leading_col.reserve(nrow);
    vec<Val> verifyMatrix;
    vec<flsss_detail::VerifyBounds<Val>> verifyBounds;
    if (nc > 1) {
        verifyMatrix.reserve(nrow * (nc - 1));
        verifyBounds.reserve(nc - 1);
        for (size_t c = 0; c < nc; ++c) {
            if (c == leadingC) continue;
            verifyBounds.push_back({
                targetSumLowerBound[c],
                targetSumUpperBound[c]});
        }
    }

    vec<Ind> rowOrder = flsss_detail::indices<Ind>(nrow);
    std::sort(rowOrder.begin(), rowOrder.end(),
        [&](auto a, auto b) {
            return X[size_t(a) * nc + leadingC]
                < X[size_t(b) * nc + leadingC];
        });
    for (size_t j = 0; j < nrow; ++j) {
        const auto src = size_t(rowOrder[j]);
        leading_col.push_back(X[src * nc + leadingC]);
        if (nc > 1) {
            for (size_t c = 0; c < nc; ++c) {
                if (c == leadingC) continue;
                verifyMatrix.push_back(X[src * nc + c]);
            }
        }
    }

    const auto loLeading = targetSumLowerBound[leadingC];
    const auto hiLeading = targetSumUpperBound[leadingC];
    vec<Ind> rootBounds =
        flsss_detail::root_index_bounds<Ind>(len, nrow);

    using VerifyT = flsss_detail::VerifyBand<
        Val, flsss_detail::verify_rest_v<Ncol>>;

    vec<vec<Ind>> solutions;
    const auto dl = deadline.value_or(
        flsss_detail::deadline_from_seconds(timeLimitSeconds));
    TriMat<Val, Ind> M;
    M.build(leading_col.data(), nrow, len);
    if (verifyBounds.empty()) {
        flsss_detail::runCoreWithMat<Val, Ind>(
            M, len, rootBounds.data(), rootBounds.data() + len,
            nrow, rowOrder.data(),
            loLeading, hiLeading,
            nSolutionsNeeded, maxIterations, dl, {},
            n_threads)
            .swap(solutions);
    } else {
        VerifyT verify;
        verify.bind(verifyMatrix.data(),
                    verifyBounds.data(),
                    verifyBounds.size());
        flsss_detail::runCoreWithMat<Val, Ind, VerifyT>(
            M, len, rootBounds.data(), rootBounds.data() + len,
            nrow, rowOrder.data(),
            loLeading, hiLeading,
            nSolutionsNeeded, maxIterations, dl, verify,
            n_threads)
            .swap(solutions);
    }
    return solutions;
}


template <typename Val, typename Ind, size_t Ncol = 0>
[[nodiscard]] vec<vec<Ind>> FLSSS_nonzero_len(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    std::optional<
        std::chrono::steady_clock::time_point> deadline = {},
    int n_threads = 1)
{
    const auto nc = flsss_detail::ncol_or<Ncol>(ncol);
    if (len == 0 || len > nrow || nrow == 0 || nc == 0)
        return {};

    const bool complement = len > nrow - len;
    const size_t k_solve = complement ? nrow - len : len;

    vec<Val> lo_work(
        targetSumLowerBound, targetSumLowerBound + nc);
    vec<Val> hi_work(
        targetSumUpperBound, targetSumUpperBound + nc);

    if (complement) {
        const auto totals = column_totals(X, nrow, nc);
        if (k_solve == 0) {
            for (size_t c = 0; c < nc; ++c)
                if (lo_work[c] > totals[c]
                    || hi_work[c] < totals[c])
                    return {};
            vec<Ind> all = flsss_detail::indices<Ind>(nrow);
            return vec<vec<Ind>>{std::move(all)};
        }
        complement_bounds_in_place(
            totals, lo_work.data(), hi_work.data(), nc);
    }

    if (!tighten_bounds_for_len(
            X, nrow, nc, k_solve,
            lo_work.data(), hi_work.data()))
        return {};

    size_t leadingC = 0;
    if (nc > 1) {
        const auto moments = make_column_moments(X, nrow, nc);
        leadingC = score_len(
            k_solve, nrow, nc, moments, X,
            lo_work.data(), hi_work.data()).leadingC;
    }

    auto subset = FLSSS_nonzero_len_with_leading<Val, Ind, Ncol>(
        X, nrow, nc, k_solve, leadingC,
        lo_work.data(), hi_work.data(),
        nSolutionsNeeded, maxIterations,
        timeLimitSeconds, deadline, n_threads);

    if (!complement) return subset;

    for (auto& s : subset)
        s = complement_indices<Ind>(nrow, s);
    return subset;
}
