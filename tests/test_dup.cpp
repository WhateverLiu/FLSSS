#include "../flsss_core.hpp"
#include <cstdio>
#include <set>
int main() {
    vec<int> a{
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 10, hi = 14;
    auto sols = FLSSS_core<int, int32_t>(
        a.data(), a.size(), 1, 0,
        &lo, &hi, 1u << 30, 0, 0.0);
    std::set<vec<int>> uniq;
    for (auto& s : sols) {
        auto t = s;
        uniq.insert(t);
    }
    std::printf(
        "raw=%zu unique=%zu\n",
        sols.size(), uniq.size());
}
