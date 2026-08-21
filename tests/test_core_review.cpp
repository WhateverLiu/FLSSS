#include "../flsss_core.hpp"
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <set>

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

int main() {
    vec<int> a{
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 10, hi = 14;
    auto sols = FLSSS_core<int, int32_t>(
        a.data(), a.size(), 1, 3,
        &lo, &hi, 1u << 30, 0, 0.0);
    std::set<vec<int>> got;
    for (auto& s : sols) {
        auto t = s;
        std::sort(t.begin(), t.end());
        got.insert(t);
    }
    auto want = brute1D(a, 3, lo, hi);
    std::printf(
        "1d got=%zu want=%zu %s\n",
        got.size(), want.size(),
        got == want ? "OK" : "FAIL");
    return got == want ? 0 : 1;
}
