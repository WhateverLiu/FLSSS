#include "../flsss_core.hpp"
#include <algorithm>
#include <cstdio>
#include <set>
int main() {
    vec<int> orig{
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 10, hi = 14;
    auto sols = FLSSS_core<int, int32_t>(
        orig.data(), orig.size(), 1, 0,
        &lo, &hi, 1u << 30, 0, 0.0);
    std::set<vec<int>> got;
    for (auto& s : sols) {
        auto t = s;
        std::sort(t.begin(), t.end());
        got.insert(t);
    }
    std::printf(
        "raw=%zu unique_sorted=%zu\n",
        sols.size(), got.size());
}
