#pragma once

#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include "flsss_gen_common.hpp"
#include "_core_solver_api.hpp"
#include "_core_api.hpp"

namespace py = pybind11;

namespace flsss_bind {

template <typename Val>
using core_fn = FLSSSGenResult (*)(
    const Val*, size_t, size_t, size_t,
    const Val*, const Val*,
    size_t, size_t, double, int);

inline py::list solutions_to_list(FLSSSGenResult&& res)
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
    size_t n_threads,
    core_fn<Val> c8,
    core_fn<Val> c16,
    core_fn<Val> c32,
    core_fn<Val> c64)
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
        res = flsss_detail::FLSSS_gen_run<Val>(
            xp, nrow, ncol, len, lp, hp,
            n_solutions, max_iterations,
            time_limit, int(n_threads),
            [&]<typename Ind>(
                const Val* x, size_t nr, size_t nc, size_t ln,
                const Val* vlo, const Val* vhi,
                size_t nsol, size_t maxit, double tlim, int nthr) {
                if constexpr (std::is_same_v<Ind, int8_t>)
                    return c8(x, nr, nc, ln, vlo, vhi, nsol, maxit, tlim, nthr);
                else if constexpr (std::is_same_v<Ind, int16_t>)
                    return c16(x, nr, nc, ln, vlo, vhi, nsol, maxit, tlim, nthr);
                else if constexpr (std::is_same_v<Ind, int32_t>)
                    return c32(x, nr, nc, ln, vlo, vhi, nsol, maxit, tlim, nthr);
                else
                    return c64(x, nr, nc, ln, vlo, vhi, nsol, maxit, tlim, nthr);
            });
    }
    return solutions_to_list(std::move(res));
}

}  // namespace flsss_bind
