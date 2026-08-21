#pragma once

#include "flsss_core.hpp"
#include "flsss_gen_common.hpp"
#include "_core_solver_api.hpp"

#ifndef FLSSS_VAL
#error "FLSSS_VAL must be defined"
#endif
#ifndef FLSSS_IND
#error "FLSSS_IND must be defined"
#endif
#ifndef FLSSS_CORE_FN
#error "FLSSS_CORE_FN must be defined"
#endif

FLSSSGenResult FLSSS_CORE_FN(
    const FLSSS_VAL* X, size_t nrow, size_t ncol, size_t len,
    const FLSSS_VAL* lo, const FLSSS_VAL* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads)
{
    return flsss_detail::make_gen_result(
        FLSSS_core<FLSSS_VAL, FLSSS_IND>(
            X, nrow, ncol, len, lo, hi,
            n_solutions, max_iterations,
            time_limit, n_threads));
}
