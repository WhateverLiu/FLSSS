#include <cstdint>
#include "_core_call.hpp"

namespace py = pybind11;

namespace {

py::list gen_dispatch(
    py::array X, py::array lo, py::array hi,
    size_t len, size_t n_solutions,
    size_t max_iterations, double time_limit,
    size_t n_threads, bool verbose)
{
    const auto dt = X.dtype();
    if (dt.is(py::dtype::of<int8_t>()))
        return flsss_bind::call_gen<int8_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads, verbose,
            flsss_core_i8_i8, flsss_core_i8_i16,
            flsss_core_i8_i32, flsss_core_i8_i64);
    if (dt.is(py::dtype::of<int16_t>()))
        return flsss_bind::call_gen<int16_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads, verbose,
            flsss_core_i16_i8, flsss_core_i16_i16,
            flsss_core_i16_i32, flsss_core_i16_i64);
    if (dt.is(py::dtype::of<int32_t>()))
        return flsss_bind::call_gen<int32_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads, verbose,
            flsss_core_i32_i8, flsss_core_i32_i16,
            flsss_core_i32_i32, flsss_core_i32_i64);
    if (dt.is(py::dtype::of<int64_t>()))
        return flsss_bind::call_gen<int64_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads, verbose,
            flsss_core_i64_i8, flsss_core_i64_i16,
            flsss_core_i64_i32, flsss_core_i64_i64);
    throw py::type_error(
        "X must be a signed integer array");
}

} // namespace

PYBIND11_MODULE(_core, m)
{
    m.doc() = "C++ bindings for FLSSS_gen";
    m.def("gen", &gen_dispatch,
        py::arg("X"), py::arg("lo"), py::arg("hi"),
        py::arg("len") = 0,
        py::arg("n_solutions") = 1,
        py::arg("max_iterations") = 0,
        py::arg("time_limit") = 0.0,
        py::arg("n_threads") = 0,
        py::arg("verbose") = false);
}
