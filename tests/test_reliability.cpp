#include "../flsss_gen.hpp"
#include "../flsss_size.hpp"
#include "../trimat.hpp"
#include <chrono>
#include <cstdio>
#include <limits>
#include <stdexcept>
#include <variant>
#include <vector>

int main() {
    for (size_t N = 1; N <= 40; ++N) {
        for (size_t L = 1; L <= N; ++L) {
            const auto got = TriMat<int, int>::valueCount(N, L);
            const auto want = (2 * N - L + 1) * L / 2;
            if (got != want) {
                std::printf("valueCount N=%zu L=%zu FAIL\n",
                    N, L);
                return 1;
            }
        }
    }
    try {
        (void)TriMat<int, int>::valueCount(1, 2);
        std::printf("L>N FAIL\n");
        return 1;
    } catch (const std::invalid_argument&) {}

    try {
        (void)flsss_detail::mul_or_throw(
            std::numeric_limits<size_t>::max(), 3);
        std::printf("mul overflow FAIL\n");
        return 1;
    } catch (const std::overflow_error&) {}

    std::vector<int> x{1, 2, 3, 4, 5};
    int lo = 3, hi = 9;
    const auto t0 = std::chrono::steady_clock::now();
    auto big = FLSSS_gen(
        x.data(), x.size(), size_t{1}, size_t{0},
        &lo, &hi, size_t(1) << 40, size_t{0}, 0.0, 1);
    const auto ms = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - t0).count();
    const auto nbig = std::visit(
        [](auto&& s) { return s.size(); }, big);
    if (nbig == 0 || ms > 5000) {
        std::printf("huge n_solutions FAIL n=%zu ms=%.1f\n",
            nbig, ms);
        return 1;
    }
    std::printf("huge n_solutions %zu in %.1f ms OK\n",
        nbig, ms);

    std::vector<int> z(22, 0);
    int zlo = 0, zhi = 0;
    const auto u0 = std::chrono::steady_clock::now();
    auto stopped = FLSSS_gen(
        z.data(), z.size(), size_t{1}, size_t{11},
        &zlo, &zhi, size_t(1) << 30, size_t{0}, 0.001, 1);
    const auto ums = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - u0).count();
    const auto nu = std::visit(
        [](auto&& s) { return s.size(); }, stopped);
    constexpr size_t nCk = 705432;
    if (nu >= nCk) {
        std::printf("deadline did not stop: %zu FAIL\n", nu);
        return 1;
    }
    std::printf("deadline %zu sols in %.2f ms OK\n", nu, ums);
    return 0;
}
