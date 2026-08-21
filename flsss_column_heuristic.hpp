#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <type_traits>
#include "flsss_common.hpp"


template <typename Val>
struct ColumnMoments {
    vec<double> mu;
    vec<double> var;
    vec<uint8_t> allEqual;
};

template <typename Val>
struct LenLeadingScore {
    size_t leadingC = 0;
    double prob = 0.0;
};

template <typename Val>
[[nodiscard]] ColumnMoments<Val> make_column_moments(
    const Val* X, size_t nrow, size_t ncol)
{
    ColumnMoments<Val> out;
    out.mu.reserve(ncol);
    out.var.reserve(ncol);
    out.allEqual.assign(ncol, 1);
    for (size_t c = 0; c < ncol; ++c) {
        out.mu.push_back(double(X[c]));
        out.var.push_back(double(X[c]) * double(X[c]));
    }
    for (size_t r = 1; r < nrow; ++r) {
        for (size_t c = 0; c < ncol; ++c) {
            const auto x = X[r * ncol + c];
            if (x != X[(r - 1) * ncol + c]) out.allEqual[c] = 0;
            const auto y = double(x);
            out.mu[c] += y;
            out.var[c] += y * y;
        }
    }
    for (size_t c = 0; c < ncol; ++c) {
        out.mu[c] /= double(nrow);
        if (out.allEqual[c]) out.var[c] = 0.0;
        else {
            out.var[c] /= double(nrow);
            out.var[c] = std::max(
                0.0, out.var[c] - out.mu[c] * out.mu[c]);
        }
    }
    return out;
}

template <typename Val>
[[nodiscard]] LenLeadingScore<Val> score_len(
    size_t len, size_t nrow, size_t ncol,
    const ColumnMoments<Val>& moments,
    const Val* X, const Val* lo, const Val* hi)
{
    LenLeadingScore<Val> out;
    out.prob = std::numeric_limits<double>::infinity();
    constexpr double invSqrt2 = 0.70710678118654752440;
    for (size_t c = 0; c < ncol; ++c) {
        auto loD = double(lo[c]);
        auto hiD = double(hi[c]);
        if constexpr (std::is_integral_v<Val>) {
            loD -= 0.999;
            hiD += 0.999;
        }

        double prob = 0.0;
        if (moments.allEqual[c]) {
            const auto meanSum = double(X[c]) * double(len);
            if (loD <= meanSum && meanSum <= hiD) prob = 1.0;
        } else {
            const auto fpc = nrow > 1
                ? double(nrow - len) / double(nrow - 1) : 0.0;
            const auto sd = std::sqrt(std::max(
                0.0, double(len) * moments.var[c] * fpc));
            const auto meanSum = moments.mu[c] * double(len);
            if (len == nrow || sd <= 0.0) {
                if (loD <= meanSum && meanSum <= hiD) prob = 1.0;
            } else {
                const auto z1 = (hiD - meanSum) / sd;
                const auto z0 = (loD - meanSum) / sd;
                prob = 0.5 * (
                    std::erf(z1 * invSqrt2)
                    - std::erf(z0 * invSqrt2));
                if (prob < 0.0) prob = 0.0;
            }
        }

        if (prob < out.prob) {
            out.prob = prob;
            out.leadingC = c;
        }
    }
    return out;
}
