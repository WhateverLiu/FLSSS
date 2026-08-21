#include "../flsss_core.hpp"
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <set>

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
            if (s >= lo && s <= hi)
                out.insert(idx);
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

int main() {
    vec<int> orig{
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 10, hi = 14;
    const int lo0 = lo, hi0 = hi;
    auto sols = FLSSS_core<int, int32_t>(
        orig.data(), orig.size(), 1, 0,
        &lo, &hi, 1u << 30, 0, 0.0);
    std::set<vec<int>> got;
    for (auto& s : sols) {
        auto t = s;
        std::sort(t.begin(), t.end());
        got.insert(t);
    }
    auto want = bruteVar(orig, lo0, hi0);
    size_t bad = 0;
    for (auto& g : got)
        if (!want.count(g)) bad++;
    for (auto& w : want)
        if (!got.count(w)) bad++;
    std::printf(
        "len0 got=%zu want=%zu bad=%zu %s\n",
        got.size(), want.size(), bad,
        bad == 0 ? "OK" : "FAIL");
    return bad == 0 ? 0 : 1;
}
