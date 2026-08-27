#pragma once
#include "flsss_column_heuristic.hpp"
#include "flsss_common.hpp"
#include "flsss_core_detail.hpp"
#include "flsss_size.hpp"
#include <algorithm>
#include <chrono>
#include <utility>


template <typename Val, typename Ind>
[[nodiscard]] vec<vec<Ind>> FLSSS_variable_len(
    const Val* X, size_t nrow, size_t ncol,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    int n_threads = 1,
    flsss_detail::FindBoundProfile* profile = nullptr)
{
    if (nrow == 0 || ncol == 0 || nSolutionsNeeded == 0)
        return {};

    const auto moments = make_column_moments(X, nrow, ncol);
    const vec<Val> lo_saved(
        targetSumLowerBound, targetSumLowerBound + ncol);
    const vec<Val> hi_saved(
        targetSumUpperBound, targetSumUpperBound + ncol);
    const auto col_totals = column_totals(X, nrow, ncol);

    struct Candidate {
        size_t solve_k = 0;
        bool flipped = false;
        bool trivial_full = false;
        vec<Val> lo;
        vec<Val> hi;
        LenLeadingScore<Val> score{};
    };

    // Input mutation: does not modify X or user-bound arrays; only
    // mutates local copies inside the returned Candidate.
    auto try_candidate = [&](
        size_t solve_k, bool flipped, bool trivial_full)
        -> std::optional<Candidate>
    {
        Candidate c{
            solve_k, flipped, trivial_full, lo_saved, hi_saved, {}};
        if (flipped)
            complement_bounds_in_place(
                col_totals, c.lo.data(), c.hi.data(), ncol);
        if (!tighten_bounds_for_len(
                X, nrow, ncol, solve_k,
                c.lo.data(), c.hi.data()))
            return std::nullopt;
        c.score = score_len(
            solve_k, nrow, ncol, moments, X,
            c.lo.data(), c.hi.data());
        return c;
    };

    vec<Candidate> ranked;
    ranked.reserve(nrow + 1);
    if (auto c = try_candidate(nrow, false, true))
        ranked.push_back(std::move(*c));
    for (size_t k = 1; k <= nrow / 2; ++k) {
        if (auto c = try_candidate(k, false, false))
            ranked.push_back(std::move(*c));
        // k == N/2: flipped also targets N/2-subsets;
        // keep original only.
        if (k < nrow - k) {
            if (auto c = try_candidate(k, true, false))
                ranked.push_back(std::move(*c));
        }
    }
    std::sort(ranked.begin(), ranked.end(),
        [](const Candidate& a, const Candidate& b) {
            return a.score.prob > b.score.prob;
        });

    const auto deadline =
        flsss_detail::deadline_from_seconds(timeLimitSeconds);

    vec<vec<Ind>> solutions;
    solutions.reserve(
        flsss_detail::result_reserve_cap(nSolutionsNeeded));
    for (auto& c : ranked) {
        if (solutions.size() >= nSolutionsNeeded) break;
        if (std::chrono::steady_clock::now() >= deadline) break;

        if (c.trivial_full) {
            solutions.push_back(flsss_detail::indices<Ind>(nrow));
            continue;
        }

        auto part = FLSSS_nonzero_len_with_leading<Val, Ind>(
            X, nrow, ncol, c.solve_k, c.score.leadingC,
            c.lo.data(), c.hi.data(),
            nSolutionsNeeded - solutions.size(),
            maxIterations, timeLimitSeconds, deadline,
            n_threads, profile);
        if (c.flipped) {
            for (auto& s : part)
                s = complement_indices<Ind>(nrow, s);
        }
        for (auto& s : part)
            solutions.push_back(std::move(s));
    }

    return solutions;
}
