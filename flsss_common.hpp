#pragma once

#include "infra/infra.hpp"
#include <chrono>
#include <cstddef>

template <typename T>
using vec = infra::sequence<T>;

namespace flsss_detail {

// Compile-time column count for FLSSS_gen dispatch.
// 0 means runtime ncol (the current path).
inline constexpr size_t kMaxStaticNcol = 10;

template <size_t Ncol>
[[nodiscard]] constexpr size_t ncol_or(size_t ncol) noexcept
{
    if constexpr (Ncol != 0) return Ncol;
    return ncol;
}

template <size_t Ncol>
inline constexpr size_t verify_rest_v =
    Ncol > 1 ? Ncol - 1 : 0;

[[nodiscard]] inline std::chrono::steady_clock::time_point
deadline_from_seconds(double t)
{
    using clock = std::chrono::steady_clock;
    if (t <= 0.0) return clock::time_point::max();
    return clock::now() + std::chrono::duration_cast<
        clock::duration>(std::chrono::duration<double>(t));
}

template <typename Ind>
[[nodiscard]] vec<Ind> indices(size_t n, size_t start = 0)
{
    auto out = vec<Ind>::uninitialized(n);
    for (size_t i = 0; i < n; ++i)
        out[i] = Ind(start + i);
    return out;
}

// LB[i] = i, UB[i] = nrow - len + i, packed as [LB | UB].
template <typename Ind>
[[nodiscard]] vec<Ind> root_index_bounds(size_t len, size_t nrow)
{
    auto out = vec<Ind>::uninitialized(2 * len);
    const auto ub0 = nrow - len;
    for (size_t i = 0; i < len; ++i) {
        out[i] = Ind(i);
        out[len + i] = Ind(ub0 + i);
    }
    return out;
}

}  // namespace flsss_detail
