#pragma once

// ===========================================================
// trimat.hpp
//
// TriMat<Val, Ind>: staircase matrix of consecutive-run
// sums for the scalar subset-sum branch-and-bound.
//
//   M(i, j) = v[j] + v[j+1] + ... + v[j+i]
//
// Row i has columns j in [0, N - i). Total stored values:
//   (2*N - L + 1) * L / 2
//
// All payload lives in one malloc block:
// [ Val cells ][ Val* row table ].
// Val must be trivially copyable with +=.
// ===========================================================

#include <cstddef>
#include <cstdlib>
#include <type_traits>

template <typename Val, typename Ind>
class TriMat {
    static_assert(std::is_trivially_copyable_v<Val>);
public:
    TriMat() = default;
    TriMat(const TriMat&) = delete;
    TriMat& operator=(const TriMat&) = delete;
    ~TriMat() { destroy(); }

    [[nodiscard]] static size_t valueCount(size_t N, size_t L) {
        return (2 * N - L + 1) * L / 2;
    }

    void build(auto&& src, size_t N, size_t L) {
        destroy();

        N_ = Ind(N);
        L_ = Ind(L);
        const auto nVal = valueCount(N, L);
        const auto valBytes = nVal * sizeof(Val);
        const auto ptrOff   = alignUp(valBytes, alignof(Val*));
        const auto total    = ptrOff + L * sizeof(Val*);

        block_ = reinterpret_cast<std::byte*>(std::malloc(total));
        if (!block_ && total != 0) return;

        auto* data = reinterpret_cast<Val*>(block_);
        rows_ = reinterpret_cast<Val**>(block_ + ptrOff);

        buildRowPointers(data);
        gatherRow0(src);
        buildRunSums();
    }

    [[nodiscard]] Val* operator[](Ind i) {
        return rows_[i];
    }
    [[nodiscard]] const Val* operator[](Ind i) const {
        return rows_[i];
    }
    [[nodiscard]] Val* values() {
        return rows_ ? rows_[0] : nullptr;
    }
    [[nodiscard]] const Val* values() const {
        return rows_ ? rows_[0] : nullptr;
    }
    [[nodiscard]] Ind N() const { return N_; }
    [[nodiscard]] Ind L() const { return L_; }

private:
    [[nodiscard]] static size_t alignUp(size_t n, size_t a) {
        return (n + (a - 1)) / a * a;
    }

    void buildRowPointers(Val* data) {
        rows_[0] = data;
        for (auto i = Ind{1}; i < L_; ++i)
            rows_[i] = rows_[i - 1] + (N_ - (i - 1));
    }

    void gatherRow0(auto&& src) {
        for (auto j = Ind{0}; j < N_; ++j)
            rows_[0][j] = src[j];
    }

    void buildRunSums() {
        for (auto i = Ind{1}; i < L_; ++i) {
            auto*       cur  = rows_[i];
            const auto* prev = rows_[i - 1];
            const auto* v0   = rows_[0];
            const auto jend = Ind(N_ - i);
            for (auto j = Ind{0}; j < jend; ++j) {
                cur[j] = prev[j];
                cur[j] += v0[i + j];
            }
        }
    }

    void destroy() {
        std::free(block_);
        block_ = nullptr;
        rows_ = nullptr;
        N_ = L_ = 0;
    }

    std::byte*  block_ = nullptr;
    Val**       rows_ = nullptr;
    Ind         N_ = 0, L_ = 0;
};
