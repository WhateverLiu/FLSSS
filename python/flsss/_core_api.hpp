#pragma once

#include <cstddef>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>

namespace py = pybind11;

py::list call_gen_i8(
    py::array X, py::array lo, py::array hi,
    size_t len, size_t n_solutions,
    size_t max_iterations, double time_limit,
    size_t n_threads);
py::list call_gen_i16(
    py::array X, py::array lo, py::array hi,
    size_t len, size_t n_solutions,
    size_t max_iterations, double time_limit,
    size_t n_threads);
py::list call_gen_i32(
    py::array X, py::array lo, py::array hi,
    size_t len, size_t n_solutions,
    size_t max_iterations, double time_limit,
    size_t n_threads);
py::list call_gen_i64(
    py::array X, py::array lo, py::array hi,
    size_t len, size_t n_solutions,
    size_t max_iterations, double time_limit,
    size_t n_threads);
