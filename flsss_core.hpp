#pragma once
#include "flsss_core_detail.hpp"
#include "flsss_variable_len.hpp"


template <typename Val, typename Ind>
[[nodiscard]] vec<vec<Ind>> FLSSS_core(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    int n_threads = 1)
{
    if (len == 0) {
        return FLSSS_variable_len<Val, Ind>(
            X, nrow, ncol,
            targetSumLowerBound, targetSumUpperBound,
            nSolutionsNeeded, maxIterations, timeLimitSeconds,
            n_threads);
    }

    return FLSSS_nonzero_len<Val, Ind>(
        X, nrow, ncol, len,
        targetSumLowerBound, targetSumUpperBound,
        nSolutionsNeeded, maxIterations, timeLimitSeconds,
        {}, n_threads);
}
