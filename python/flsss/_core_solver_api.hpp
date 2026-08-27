#pragma once

#include <cstddef>
#include <cstdint>
#include "flsss_gen_common.hpp"

FLSSSGenResult flsss_core_i8_i8(
    const int8_t* X, size_t nrow, size_t ncol, size_t len,
    const int8_t* lo, const int8_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i8_i16(
    const int8_t* X, size_t nrow, size_t ncol, size_t len,
    const int8_t* lo, const int8_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i8_i32(
    const int8_t* X, size_t nrow, size_t ncol, size_t len,
    const int8_t* lo, const int8_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i8_i64(
    const int8_t* X, size_t nrow, size_t ncol, size_t len,
    const int8_t* lo, const int8_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);

FLSSSGenResult flsss_core_i16_i8(
    const int16_t* X, size_t nrow, size_t ncol, size_t len,
    const int16_t* lo, const int16_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i16_i16(
    const int16_t* X, size_t nrow, size_t ncol, size_t len,
    const int16_t* lo, const int16_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i16_i32(
    const int16_t* X, size_t nrow, size_t ncol, size_t len,
    const int16_t* lo, const int16_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i16_i64(
    const int16_t* X, size_t nrow, size_t ncol, size_t len,
    const int16_t* lo, const int16_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);

FLSSSGenResult flsss_core_i32_i8(
    const int32_t* X, size_t nrow, size_t ncol, size_t len,
    const int32_t* lo, const int32_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i32_i16(
    const int32_t* X, size_t nrow, size_t ncol, size_t len,
    const int32_t* lo, const int32_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i32_i32(
    const int32_t* X, size_t nrow, size_t ncol, size_t len,
    const int32_t* lo, const int32_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i32_i64(
    const int32_t* X, size_t nrow, size_t ncol, size_t len,
    const int32_t* lo, const int32_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);

FLSSSGenResult flsss_core_i64_i8(
    const int64_t* X, size_t nrow, size_t ncol, size_t len,
    const int64_t* lo, const int64_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i64_i16(
    const int64_t* X, size_t nrow, size_t ncol, size_t len,
    const int64_t* lo, const int64_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i64_i32(
    const int64_t* X, size_t nrow, size_t ncol, size_t len,
    const int64_t* lo, const int64_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
FLSSSGenResult flsss_core_i64_i64(
    const int64_t* X, size_t nrow, size_t ncol, size_t len,
    const int64_t* lo, const int64_t* hi,
    size_t n_solutions, size_t max_iterations,
    double time_limit, int n_threads, bool verbose);
