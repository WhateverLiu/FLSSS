#pragma once
#include "flsss_core.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <variant>


using FLSSSGenResult = std::variant<
    vec<vec<int8_t>>,
    vec<vec<int16_t>>,
    vec<vec<int32_t>>,
    vec<vec<int64_t>>>;


namespace flsss_detail {

inline constexpr auto kI32Max = uint64_t(
    std::numeric_limits<int32_t>::max());

[[nodiscard]] constexpr size_t minSignedByteSizeIndexCount(size_t n)
{
    if (n > kI32Max) return sizeof(int64_t);
    if (n <= 127) return sizeof(int8_t);
    if (n <= 32767) return sizeof(int16_t);
    return sizeof(int32_t);
}

template <typename Ind>
[[nodiscard]] FLSSSGenResult make_gen_result(vec<vec<Ind>> sols)
{
    return FLSSSGenResult{
        std::in_place_type<vec<vec<Ind>>>,
        std::move(sols)};
}

template <typename Val>
void ensureMatrixValueWidth(double maxColAbsSum)
{
    constexpr auto limit =
        double(std::numeric_limits<Val>::max()) * 0.999;
    if (!std::isfinite(maxColAbsSum) || maxColAbsSum > limit)
        throw std::runtime_error(
            "Input matrix column sums exceed "
            "the value type's range.");
}

[[nodiscard]] auto dispatchSignedWidth(size_t b, auto&& fn)
{
    if (b == sizeof(int8_t))
        return fn.template operator()<int8_t>();
    if (b == sizeof(int16_t))
        return fn.template operator()<int16_t>();
    if (b == sizeof(int32_t))
        return fn.template operator()<int32_t>();
    return fn.template operator()<int64_t>();
}
} // namespace flsss_detail


template <typename Val>
[[nodiscard]] FLSSSGenResult FLSSS_gen(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    int n_threads = 1)
{
    static_assert(
        std::is_integral_v<Val>
        && std::is_signed_v<Val>
        && !std::is_same_v<Val, bool>,
        "FLSSS_gen requires a signed integer value type.");

    if (nrow == 0 || ncol == 0 || nSolutionsNeeded == 0) {
        return flsss_detail::dispatchSignedWidth(
            flsss_detail::minSignedByteSizeIndexCount(nrow),
            []<typename Ind>() {
                return flsss_detail::make_gen_result(
                    vec<vec<Ind>>{});
            });
    }

    vec<double> colAbsSums(ncol);
    for (size_t r = 0; r < nrow; ++r) {
        const auto* row = X + r * ncol;
        for (size_t c = 0; c < ncol; ++c)
            colAbsSums[c] += std::abs(double(row[c]));
    }
    for (size_t c = 0; c < ncol; ++c) {
        double t = std::max(
            std::abs(double(targetSumLowerBound[c])),
            std::abs(double(targetSumUpperBound[c])));
        colAbsSums[c] += t;
    }
    double maxColSum = *std::max_element(
        colAbsSums.begin(), colAbsSums.end());

    flsss_detail::ensureMatrixValueWidth<Val>(maxColSum);

    return flsss_detail::dispatchSignedWidth(
        flsss_detail::minSignedByteSizeIndexCount(nrow),
        [&]<typename Ind>() {
            return flsss_detail::make_gen_result(
                FLSSS_core<Val, Ind>(
                    X, nrow, ncol, len,
                    targetSumLowerBound,
                    targetSumUpperBound,
                    nSolutionsNeeded, maxIterations,
                    timeLimitSeconds, n_threads));
        });
}




