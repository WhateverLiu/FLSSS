#pragma once
#include "flsss_common.hpp"
#include <algorithm>
#include <cstddef>
#include <thread>
#include <utility>


namespace flsss_brute_detail {

template <typename Val>
[[nodiscard]] bool column_sums_in_band(
    const Val* sums, size_t ncol,
    const Val* lo, const Val* hi)
{
    for (size_t c = 0; c < ncol; ++c)
        if (sums[c] < lo[c] || sums[c] > hi[c])
            return false;
    return true;
}

template <typename Val, typename Ind>
void initialize_subset_sums(
    const Val* X, size_t ncol,
    const Ind* idx, size_t len,
    Val* sums)
{
    std::fill(sums, sums + ncol, Val{});
    for (size_t j = 0; j < len; ++j) {
        const Val* row = X + size_t(idx[j]) * ncol;
        for (size_t c = 0; c < ncol; ++c)
            sums[c] += row[c];
    }
}

template <bool Add, typename Val, typename Ind>
void update_subset_sums(
    const Val* X, size_t ncol,
    const Ind* idx, size_t begin, size_t len,
    Val* sums)
{
    for (size_t j = begin; j < len; ++j) {
        const Val* row = X + size_t(idx[j]) * ncol;
        for (size_t c = 0; c < ncol; ++c) {
            if constexpr (Add) sums[c] += row[c];
            else sums[c] -= row[c];
        }
    }
}

template <typename Val, typename Ind>
void enumerate_fixed_len_range(
    const Val* X, size_t nrow, size_t ncol, size_t len,
    const Val* lo, const Val* hi,
    Ind first_begin, Ind first_end,
    vec<vec<Ind>>& out)
{
    if (len == 0 || len > nrow) return;

    auto sums = vec<Val>::uninitialized(ncol);
    auto idx = vec<Ind>::uninitialized(len);
    const Ind first_last = Ind(nrow - len);

    for (Ind fb = first_begin;
         fb < first_end && fb <= first_last; ++fb) {
        if (len == 1) {
            const Val* row = X + size_t(fb) * ncol;
            if (column_sums_in_band(row, ncol, lo, hi))
                out.push_back(vec<Ind>{fb});
            continue;
        }

        idx[0] = fb;
        for (size_t i = 1; i < len; ++i)
            idx[i] = fb + Ind(i);
        initialize_subset_sums(
            X, ncol, idx.data(), len, sums.data());

        while (true) {
            if (column_sums_in_band(sums.data(), ncol, lo, hi))
                out.push_back(idx);

            size_t pos = len - 1;
            while (pos > 0
                   && idx[pos] == Ind(nrow) - Ind(len - pos))
                --pos;
            if (pos == 0) break;
            update_subset_sums<false>(
                X, ncol, idx.data(), pos, len, sums.data());
            ++idx[pos];
            for (size_t j = pos + 1; j < len; ++j)
                idx[j] = idx[j - 1] + 1;
            update_subset_sums<true>(
                X, ncol, idx.data(), pos, len, sums.data());
        }
    }
}

template <typename Val, typename Ind>
void enumerate_fixed_len_all(
    const Val* X, size_t nrow, size_t ncol, size_t len,
    const Val* lo, const Val* hi,
    vec<vec<Ind>>& out)
{
    if (len == 0 || len > nrow) return;
    const Ind span = Ind(nrow - len + 1);
    enumerate_fixed_len_range<Val, Ind>(
        X, nrow, ncol, len, lo, hi,
        Ind{0}, span, out);
}

template <typename Ind>
[[nodiscard]] vec<Ind> make_prefix_tasks(
    size_t nrow, size_t len, size_t depth)
{
    const size_t prefix_span = nrow - len + depth;
    auto current = vec<Ind>::uninitialized(depth);
    for (size_t i = 0; i < depth; ++i)
        current[i] = Ind(i);

    vec<Ind> tasks;
    while (true) {
        tasks.insert(tasks.end(), current.begin(), current.end());

        size_t pos = depth;
        bool advanced = false;
        while (pos > 0) {
            --pos;
            const auto max_at_pos = Ind(prefix_span - depth + pos);
            if (current[pos] < max_at_pos) {
                advanced = true;
                break;
            }
        }
        if (!advanced) break;

        ++current[pos];
        for (size_t i = pos + 1; i < depth; ++i)
            current[i] = current[i - 1] + 1;
    }
    return tasks;
}

template <typename Val, typename Ind>
void enumerate_fixed_len_prefix(
    const Val* X, size_t nrow, size_t ncol, size_t len,
    const Val* lo, const Val* hi,
    const Ind* prefix, size_t depth,
    vec<Ind>& idx, vec<Val>& sums,
    vec<vec<Ind>>& out)
{
    std::copy(prefix, prefix + depth, idx.begin());
    for (size_t i = depth; i < len; ++i)
        idx[i] = idx[i - 1] + 1;
    initialize_subset_sums(
        X, ncol, idx.data(), len, sums.data());

    while (true) {
        if (column_sums_in_band(sums.data(), ncol, lo, hi))
            out.push_back(idx);

        size_t pos = len;
        bool advanced = false;
        while (pos > depth) {
            --pos;
            if (idx[pos] < Ind(nrow - len + pos)) {
                advanced = true;
                break;
            }
        }
        if (!advanced) break;

        update_subset_sums<false>(
            X, ncol, idx.data(), pos, len, sums.data());
        ++idx[pos];
        for (size_t i = pos + 1; i < len; ++i)
            idx[i] = idx[i - 1] + 1;
        update_subset_sums<true>(
            X, ncol, idx.data(), pos, len, sums.data());
    }
}

[[nodiscard]] inline size_t resolve_thread_count(size_t numThreads)
{
    if (numThreads > 0) return numThreads;
    const auto hw = std::thread::hardware_concurrency();
    return hw > 0 ? hw : 1;
}

template <typename Val, typename Ind>
[[nodiscard]] vec<vec<Ind>> merge_thread_results(
    vec<vec<vec<Ind>>>& per_thread)
{
    size_t total = 0;
    for (const auto& part : per_thread)
        total += part.size();
    vec<vec<Ind>> out;
    out.reserve(total);
    for (auto& part : per_thread) {
        for (auto& sol : part)
            out.push_back(std::move(sol));
    }
    return out;
}

} // namespace flsss_brute_detail


// Input mutation: does not modify X, targetSumLowerBound, or
// targetSumUpperBound (read-only through their pointer ranges).
// Each thread collects into its own buffer;
// no locks during enumeration.
template <typename Val, typename Ind>
[[nodiscard]] vec<vec<Ind>> FLSSS_brute_fixed_len(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t numThreads = 0)
{
    if (nrow == 0 || ncol == 0 || len == 0 || len > nrow)
        return {};

    const size_t T =
        flsss_brute_detail::resolve_thread_count(numThreads);
    const Ind span = Ind(nrow - len + 1);
    if (T == 1) {
        vec<vec<Ind>> out;
        flsss_brute_detail::enumerate_fixed_len_range<Val, Ind>(
            X, nrow, ncol, len,
            targetSumLowerBound, targetSumUpperBound,
            Ind{0}, span, out);
        return out;
    }

    const size_t prefix_depth = len < 3 ? 1 : 3;
    const auto tasks = flsss_brute_detail::make_prefix_tasks<Ind>(
        nrow, len, prefix_depth);
    const size_t task_count = tasks.size() / prefix_depth;
    const size_t pool = std::max(
        size_t{1}, size_t(parlay::num_workers()));
    const size_t workers = std::max(
        size_t{1}, std::min({T, task_count, pool}));

    vec<vec<vec<Ind>>> per_thread(workers);
    parlay::parallel_for(0, workers, [&](size_t t) {
        auto idx = vec<Ind>::uninitialized(len);
        auto sums = vec<Val>::uninitialized(ncol);
        for (size_t task = t; task < task_count;
             task += workers) {
            flsss_brute_detail::enumerate_fixed_len_prefix<
                Val, Ind>(
                X, nrow, ncol, len,
                targetSumLowerBound, targetSumUpperBound,
                tasks.data() + task * prefix_depth,
                prefix_depth,
                idx, sums, per_thread[t]);
        }
    }, 1);
    return flsss_brute_detail::merge_thread_results<
        Val, Ind>(per_thread);
}


// Input mutation: does not modify X, targetSumLowerBound, or
// targetSumUpperBound (read-only through their pointer ranges).
// len == 0 searches every subset size from 1 through nrow.
template <typename Val, typename Ind>
[[nodiscard]] vec<vec<Ind>> FLSSS_brute(
    const Val* X, size_t nrow, size_t ncol,
    size_t len,
    const Val* targetSumLowerBound,
    const Val* targetSumUpperBound,
    size_t numThreads = 0)
{
    if (len != 0) {
        return FLSSS_brute_fixed_len<Val, Ind>(
            X, nrow, ncol, len,
            targetSumLowerBound, targetSumUpperBound,
            numThreads);
    }

    if (nrow == 0 || ncol == 0)
        return {};

    vec<vec<Ind>> out;
    const size_t T =
        flsss_brute_detail::resolve_thread_count(numThreads);
    if (T == 1) {
        for (size_t k = 1; k <= nrow; ++k)
            flsss_brute_detail::enumerate_fixed_len_all<Val, Ind>(
                X, nrow, ncol, k,
                targetSumLowerBound, targetSumUpperBound,
                out);
        return out;
    }

    for (size_t k = 1; k <= nrow; ++k) {
        auto part = FLSSS_brute_fixed_len<Val, Ind>(
            X, nrow, ncol, k,
            targetSumLowerBound, targetSumUpperBound,
            T);
        out.reserve(out.size() + part.size());
        for (auto& sol : part)
            out.push_back(std::move(sol));
    }
    return out;
}
