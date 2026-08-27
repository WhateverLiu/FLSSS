// Local-only combined instantiation TU. Compiling all four Val
// instantiations in a single TU keeps parlay's `static inline
// thread_local worker_info` to one object file, avoiding the macOS
// duplicate-symbol link error (macOS ld has no
// --allow-multiple-definition). Not part of the shipped build; used only
// by tests/build_local.sh to build the extension for the test suite.
//
// _core_inst.hpp has `#pragma once`, so we cannot include it four times
// in one TU. Instead we replicate its definition macro here.
#include <cstdint>
#include "flsss_core.hpp"
#include "flsss_gen_common.hpp"
#include "_core_solver_api.hpp"

#define FLSSS_CAT2(a, b) a##_##b
#define FLSSS_CAT(a, b) FLSSS_CAT2(a, b)

#define FLSSS_DEFINE_ONE(VAL, VSUF, IND_T, IND_SUF)                          \
  FLSSSGenResult FLSSS_CAT(FLSSS_CAT(flsss_core, VSUF), IND_SUF)(            \
      const VAL* X, size_t nrow, size_t ncol, size_t len,                   \
      const VAL* lo, const VAL* hi,                                         \
      size_t n_solutions, size_t max_iterations,                            \
      double time_limit, int n_threads, bool verbose) {                     \
    return flsss_detail::make_gen_result(                                   \
        FLSSS_core<VAL, IND_T>(                                             \
            X, nrow, ncol, len, lo, hi,                                     \
            n_solutions, max_iterations, time_limit,                        \
            n_threads, verbose));                                           \
  }

#define FLSSS_DEFINE_VAL(VAL, VSUF)                                          \
  FLSSS_DEFINE_ONE(VAL, VSUF, int8_t, i8)                                    \
  FLSSS_DEFINE_ONE(VAL, VSUF, int16_t, i16)                                  \
  FLSSS_DEFINE_ONE(VAL, VSUF, int32_t, i32)                                  \
  FLSSS_DEFINE_ONE(VAL, VSUF, int64_t, i64)

FLSSS_DEFINE_VAL(int8_t, i8)
FLSSS_DEFINE_VAL(int16_t, i16)
FLSSS_DEFINE_VAL(int32_t, i32)
FLSSS_DEFINE_VAL(int64_t, i64)
