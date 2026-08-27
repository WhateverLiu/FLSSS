#pragma once

#include "flsss_core.hpp"
#include "flsss_gen_common.hpp"
#include "_core_solver_api.hpp"

// Defines all four Ind instantiations of FLSSS_core for a single Val type.
// Each per-Val translation unit (_core_i8.cpp, ...) defines FLSSS_VAL and
// FLSSS_VAL_SUF and includes this header. Grouping the four Ind variants per
// TU (instead of 16 separate TUs) matches the ~4-vCPU CI runners: it keeps
// MSVC /MP parallelism while paying the heavy header-parse cost only 4 times
// instead of 16.

#ifndef FLSSS_VAL
#error "FLSSS_VAL must be defined"
#endif
#ifndef FLSSS_VAL_SUF
#error "FLSSS_VAL_SUF must be defined"
#endif

#define FLSSS_CAT2(a, b) a##_##b
#define FLSSS_CAT(a, b) FLSSS_CAT2(a, b)

#define FLSSS_DEFINE_CORE(IND_T, IND_SUF)                                    \
  FLSSSGenResult FLSSS_CAT(FLSSS_CAT(flsss_core, FLSSS_VAL_SUF), IND_SUF)(   \
      const FLSSS_VAL* X, size_t nrow, size_t ncol, size_t len,             \
      const FLSSS_VAL* lo, const FLSSS_VAL* hi,                             \
      size_t n_solutions, size_t max_iterations,                           \
      double time_limit, int n_threads, bool verbose) {                    \
    return flsss_detail::make_gen_result(                                   \
        FLSSS_core<FLSSS_VAL, IND_T>(                                       \
            X, nrow, ncol, len, lo, hi,                                     \
            n_solutions, max_iterations, time_limit,                        \
            n_threads, verbose));                                           \
  }

FLSSS_DEFINE_CORE(int8_t, i8)
FLSSS_DEFINE_CORE(int16_t, i16)
FLSSS_DEFINE_CORE(int32_t, i32)
FLSSS_DEFINE_CORE(int64_t, i64)

#undef FLSSS_DEFINE_CORE
#undef FLSSS_CAT
#undef FLSSS_CAT2
