#include "../flsss_gen.hpp"
#include "../flsss_core.hpp"
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <set>
#include <thread>

static std::set<vec<int>> bruteVar(
    const vec<int>& a, int lo, int hi)
{
    std::set<vec<int>> out;
    const auto n = a.size();
    for (size_t len = 1; len <= n; ++len) {
        vec<int> idx(len);
        std::iota(idx.begin(), idx.end(), 0);
        while (true) {
            int s = 0;
            for (auto i : idx) s += a[i];
            if (s >= lo && s <= hi) out.insert(idx);
            int i = int(len) - 1;
            while (i >= 0
                && idx[i] == int(n) - int(len) + i)
                --i;
            if (i < 0) break;
            ++idx[i];
            for (int j = i + 1; j < int(len); ++j)
                idx[j] = idx[j - 1] + 1;
        }
    }
    return out;
}

static std::set<vec<int>> brute1D(
    const vec<int>& orig, size_t len,
    int lo, int hi)
{
    std::set<vec<int>> out;
    vec<int> idx(len);
    std::iota(idx.begin(), idx.end(), 0);
    const auto N = orig.size();
    while (true) {
        int s = 0;
        for (auto i : idx) s += orig[i];
        if (s >= lo && s <= hi) out.insert(idx);
        int i = int(len) - 1;
        while (i >= 0
            && idx[i] == int(N) - int(len) + i)
            --i;
        if (i < 0) break;
        ++idx[i];
        for (int j = i + 1; j < int(len); ++j)
            idx[j] = idx[j - 1] + 1;
    }
    return out;
}

template <typename Sols>
static std::set<vec<int>> asSortedSets(
    const Sols& sols)
{
    std::set<vec<int>> got;
    for (const auto& sol : sols) {
        auto t = vec<int>(
            sol.begin(), sol.end());
        std::sort(t.begin(), t.end());
        got.insert(std::move(t));
    }
    return got;
}

int main()
{
    vec<int> orig{
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 10, hi = 14;
    const int lo0 = lo, hi0 = hi;

    auto res = FLSSS_gen(
        orig.data(), orig.size(), 1, 0,
        &lo, &hi, 1u << 30, 0, 0.0);
    const auto got = std::visit(
        [](const auto& sols) {
            return asSortedSets(sols);
        }, res);
    const auto want = bruteVar(orig, lo0, hi0);
    auto bad = size_t(got.size() != want.size()
        || got != want);
    std::printf(
        "gen len0 got=%zu want=%zu %s\n",
        got.size(), want.size(),
        bad ? "FAIL" : "OK");

    const int nt = std::max(2, int(
        std::thread::hardware_concurrency()));
    lo = lo0; hi = hi0;
    auto res_par = FLSSS_gen(
        orig.data(), orig.size(), 1, 0,
        &lo, &hi, 1u << 30, 0, 0.0, nt);
    const auto got_par = std::visit(
        [](const auto& sols) {
            return asSortedSets(sols);
        }, res_par);
    if (got_par != want) {
        ++bad;
        std::printf("gen len0 par FAIL\n");
    } else {
        std::printf("gen len0 par nt=%d OK\n", nt);
    }

    lo = lo0; hi = hi0;
    auto seq = FLSSS_core<int, int32_t>(
        orig.data(), orig.size(), 1, 3,
        &lo, &hi, 1u << 30, 0, 0.0, 1);
    lo = lo0; hi = hi0;
    auto par = FLSSS_core<int, int32_t>(
        orig.data(), orig.size(), 1, 3,
        &lo, &hi, 1u << 30, 0, 0.0, nt);
    auto seqs = asSortedSets(seq);
    auto pars = asSortedSets(par);
    auto want3 = brute1D(orig, 3, lo0, hi0);
    if (seqs != want3 || pars != want3) {
        ++bad;
        std::printf(
            "core len3 seq=%zu par=%zu want=%zu FAIL\n",
            seqs.size(), pars.size(), want3.size());
    } else {
        std::printf("core len3 par nt=%d OK\n", nt);
    }

    vec<int> two{
        3,1,  1,2,  4,0,  1,3,  5,1,
        9,2,  2,1,  6,0,  5,2,  3,1,
        5,0,  8,1};
    int lo2[] = {10, 3};
    int hi2[] = {14, 6};
    int lo2s[] = {10, 3};
    int hi2s[] = {14, 6};
    auto seq2 = FLSSS_core<int, int32_t>(
        two.data(), 12, 2, 3,
        lo2, hi2, 1u << 30, 0, 0.0, 1);
    auto par2 = FLSSS_core<int, int32_t>(
        two.data(), 12, 2, 3,
        lo2s, hi2s, 1u << 30, 0, 0.0, nt);
    if (asSortedSets(seq2) != asSortedSets(par2)) {
        ++bad;
        std::printf("core 2col FAIL seq=%zu par=%zu\n",
            seq2.size(), par2.size());
    } else {
        std::printf("core 2col par nt=%d n=%zu OK\n",
            nt, seq2.size());
    }

    int lo2g[] = {10, 3};
    int hi2g[] = {14, 6};
    auto gen2 = FLSSS_gen(
        two.data(), 12, 2, 3,
        lo2g, hi2g, 1u << 30, 0, 0.0, 1);
    const auto gen2s = std::visit(
        [](const auto& sols) {
            return asSortedSets(sols);
        }, gen2);
    if (gen2s != asSortedSets(seq2)) {
        ++bad;
        std::printf("gen 2col FAIL gen=%zu core=%zu\n",
            gen2s.size(), seq2.size());
    } else {
        std::printf("gen 2col n=%zu OK\n", gen2s.size());
    }

    constexpr int nrw = 5, ncw = 11, kw = 2;
    vec<int> wide;
    wide.reserve(nrw * ncw);
    for (int r = 0; r < nrw; ++r)
        for (int c = 0; c < ncw; ++c)
            wide.push_back(1 + (r * ncw + c) % 7);
    vec<int> lo11(ncw, 2), hi11(ncw, 14);
    vec<int> lo11c(ncw, 2), hi11c(ncw, 14);
    auto gen11 = FLSSS_gen(
        wide.data(), nrw, ncw, kw,
        lo11.data(), hi11.data(), 1u << 30, 0, 0.0);
    auto core11 = FLSSS_core<int, int32_t>(
        wide.data(), nrw, ncw, kw,
        lo11c.data(), hi11c.data(), 1u << 30, 0, 0.0);
    const auto gen11s = std::visit(
        [](const auto& sols) {
            return asSortedSets(sols);
        }, gen11);
    if (gen11s != asSortedSets(core11)) {
        ++bad;
        std::printf("gen 11col FAIL gen=%zu core=%zu\n",
            gen11s.size(), core11.size());
    } else {
        std::printf("gen 11col n=%zu OK\n", gen11s.size());
    }

    vec<int> big;
    big.reserve(28);
    for (int i = 0; i < 28; ++i)
        big.push_back(1 + (i * 7) % 19);
    int blo = 40, bhi = 48;
    int blo2 = 40, bhi2 = 48;
    auto seqb = FLSSS_core<int, int32_t>(
        big.data(), big.size(), 1, 6,
        &blo, &bhi, 1u << 30, 0, 0.0, 1);
    auto parb = FLSSS_core<int, int32_t>(
        big.data(), big.size(), 1, 6,
        &blo2, &bhi2, 1u << 30, 0, 0.0, nt);
    if (asSortedSets(seqb) != asSortedSets(parb)) {
        ++bad;
        std::printf("core big FAIL seq=%zu par=%zu\n",
            seqb.size(), parb.size());
    } else {
        std::printf("core big par nt=%d n=%zu OK\n",
            nt, seqb.size());
    }

    int one_lo = lo0, one_hi = hi0;
    auto one = FLSSS_core<int, int32_t>(
        orig.data(), orig.size(), 1, 3,
        &one_lo, &one_hi, 1, 0, 0.0, nt);
    if (one.size() != 1) {
        ++bad;
        std::printf("core need1 par size=%zu FAIL\n",
            one.size());
    } else {
        std::printf("core need1 par OK\n");
    }

    return bad ? 1 : 0;
}
