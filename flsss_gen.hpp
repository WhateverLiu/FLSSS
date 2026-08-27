#pragma once
#include "flsss_gen_common.hpp"
#include "flsss_core.hpp"


template <typename Val>
[[nodiscard]] FLSSSGenResult FLSSS_gen(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    int n_threads = 1,
    bool verbose = false)
{
    return flsss_detail::FLSSS_gen_run<Val>(
        X, nrow, ncol, len,
        targetSumLowerBound, targetSumUpperBound,
        nSolutionsNeeded, maxIterations,
        timeLimitSeconds, n_threads,
        [verbose]<typename Ind>(
            const Val* x, size_t nr, size_t nc, size_t ln,
            const Val* lo, const Val* hi,
            size_t nsol, size_t maxit, double tlim, int nthr) {
            return flsss_detail::make_gen_result(
                FLSSS_core<Val, Ind>(
                    x, nr, nc, ln, lo, hi,
                    nsol, maxit, tlim, nthr, verbose));
        });
}
