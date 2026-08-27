#pragma once
#include "flsss_core_detail.hpp"
#include "flsss_variable_len.hpp"

#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>


namespace flsss_detail {

// Print the verbose findBound() timing report to std::cerr.
//
// overallNs is the wall-clock duration of the whole gen() computation
// (sorting, bound tightening, matrix build, and the branch-and-bound
// solve). Per-length times are the AVERAGE across every worker thread
// ever spawned; each length's percentage is its share of the overall
// time, so the per-length percentages sum to findBound's overall
// percentage.
inline void printFindBoundProfile(
    const FindBoundProfile& prof, uint64_t overallNs)
{
    std::ostream& os = std::cerr;
    const auto p = prof.participants;
    os << "[flsss] findBound() timing (averaged over "
       << p << " worker thread"
       << (p == 1 ? "" : "s") << " ever spawned)\n";

    if (p == 0 || overallNs == 0) {
        os << "  no findBound() calls were recorded.\n";
        os.flush();
        return;
    }

    const double ovMs = double(overallNs) / 1e6;
    auto pct = [&](double ns) { return 100.0 * ns / double(overallNs); };

    const auto oldFlags = os.flags();
    const auto oldPrec = os.precision();
    os << std::fixed << std::setprecision(3);

    os << "  overall wall time: " << ovMs << " ms\n";
    os << "  per bounding-vector length (avg time, % of overall):\n";

    long double sumAvg = 0.0L;
    for (size_t L = 0; L < prof.nsByLen.size(); ++L) {
        if (prof.nsByLen[L] == 0) continue;
        const double avgNs = double(prof.nsByLen[L]) / double(p);
        sumAvg += avgNs;
        os << "    len " << std::setw(4) << L << ": "
           << std::setw(10) << (avgNs / 1e6) << " ms  ("
           << std::setw(6) << pct(avgNs) << "%)\n";
    }

    const double fbAvgNs = double(sumAvg);
    os << "  findBound() total: " << (fbAvgNs / 1e6) << " ms  ("
       << pct(fbAvgNs) << "% of overall)\n";

    os.flags(oldFlags);
    os.precision(oldPrec);
    os.flush();
}

}  // namespace flsss_detail


template <typename Val, typename Ind>
[[nodiscard]] vec<vec<Ind>> FLSSS_core(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t nSolutionsNeeded,
    size_t maxIterations,
    double timeLimitSeconds,
    int n_threads = 1,
    bool verbose = false)
{
    flsss_detail::FindBoundProfile prof;
    flsss_detail::FindBoundProfile* pp = verbose ? &prof : nullptr;
    const auto t0 = std::chrono::steady_clock::now();

    auto solutions = (len == 0)
        ? FLSSS_variable_len<Val, Ind>(
              X, nrow, ncol,
              targetSumLowerBound, targetSumUpperBound,
              nSolutionsNeeded, maxIterations, timeLimitSeconds,
              n_threads, pp)
        : FLSSS_nonzero_len<Val, Ind>(
              X, nrow, ncol, len,
              targetSumLowerBound, targetSumUpperBound,
              nSolutionsNeeded, maxIterations, timeLimitSeconds,
              {}, n_threads, pp);

    if (verbose) {
        const auto t1 = std::chrono::steady_clock::now();
        const auto overallNs = uint64_t(
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                t1 - t0).count());
        flsss_detail::printFindBoundProfile(prof, overallNs);
    }

    return solutions;
}
