#pragma once

// Setup-path size arithmetic. Not for findBound / emit loops.
#include <cstddef>
#include <limits>
#include <stdexcept>

namespace flsss_detail {

[[nodiscard]] inline size_t mul_or_throw(size_t a, size_t b)
{
    if (b != 0 && a > std::numeric_limits<size_t>::max() / b)
        throw std::overflow_error(
            "FLSSS allocation size overflow");
    return a * b;
}

[[nodiscard]] inline size_t add_or_throw(size_t a, size_t b)
{
    if (a > std::numeric_limits<size_t>::max() - b)
        throw std::overflow_error(
            "FLSSS allocation size overflow");
    return a + b;
}

[[nodiscard]] inline size_t result_reserve_cap(size_t n)
{
    return n < 1024 ? n + 7 : 1024;
}

}  // namespace flsss_detail
