#include "../flsss_solver.hpp"
#include <cstdint>
int main() {
    vec<int32_t> rb{
        0,1,2, 9,10,11};
    vec<int32_t> ro(12);
    for (int i = 0; i < 12; ++i) ro[i] = i;
    int v[12] = {
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 10, hi = 14;
    auto s = flsss_detail::runCore<int, int32_t>(
        3, rb.data(), rb.data() + 3, v, 12,
        ro.data(), lo, hi, 100, 0, 0.0);
    return 0;
}
