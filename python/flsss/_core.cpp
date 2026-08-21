#include <algorithm>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include "flsss_gen.hpp"

namespace py = pybind11;

namespace {

py::list solutions_to_list(FLSSSGenResult&& res)
{
    return std::visit([](auto&& sols) {
        py::list out;
        using Sol = typename std::decay_t<
            decltype(sols)>::value_type;
        using Ind = typename Sol::value_type;
        for (auto& s : sols) {
            py::array_t<Ind> a(
                static_cast<py::ssize_t>(s.size()));
            auto* dst = a.mutable_data();
            std::copy(s.begin(), s.end(), dst);
            out.append(std::move(a));
        }
        return out;
    }, res);
}

template <typename Val>
py::list call_gen(
    py::array_t<Val, py::array::c_style |
        py::array::forcecast> X,
    py::array_t<Val, py::array::c_style |
        py::array::forcecast> lo,
    py::array_t<Val, py::array::c_style |
        py::array::forcecast> hi,
    size_t len,
    size_t n_solutions,
    size_t max_iterations,
    double time_limit,
    size_t n_threads)
{
    const auto xb = X.request();
    const auto lb = lo.request();
    const auto hb = hi.request();
    if (xb.ndim != 1 && xb.ndim != 2)
        throw std::invalid_argument(
            "X must be 1-D or 2-D");

    const size_t nrow = size_t(xb.shape[0]);
    const size_t ncol = xb.ndim == 1
        ? size_t{1} : size_t(xb.shape[1]);
    if (size_t(lb.size) != ncol
        || size_t(hb.size) != ncol)
        throw std::invalid_argument(
            "lo and hi must have length ncol");

    auto* xp = static_cast<Val*>(xb.ptr);
    auto* lp = static_cast<Val*>(lb.ptr);
    auto* hp = static_cast<Val*>(hb.ptr);
    FLSSSGenResult res;
    {
        py::gil_scoped_release release;
        res = FLSSS_gen(
            xp, nrow, ncol, len,
            lp, hp,
            n_solutions, max_iterations,
            time_limit, n_threads);
    }
    return solutions_to_list(std::move(res));
}

py::list gen_dispatch(
    py::array X, py::array lo, py::array hi,
    size_t len, size_t n_solutions,
    size_t max_iterations, double time_limit,
    size_t n_threads)
{
    const auto dt = X.dtype();
    if (dt.is(py::dtype::of<int8_t>()))
        return call_gen<int8_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads);
    if (dt.is(py::dtype::of<int16_t>()))
        return call_gen<int16_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads);
    if (dt.is(py::dtype::of<int32_t>()))
        return call_gen<int32_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads);
    if (dt.is(py::dtype::of<int64_t>()))
        return call_gen<int64_t>(
            X, lo, hi, len, n_solutions,
            max_iterations, time_limit, n_threads);
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
        py::arg("n_threads") = 1);
}
