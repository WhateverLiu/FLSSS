#include "../flsss_core.hpp"
#include <numeric>
#include <span>
#include <cstdint>
int main() {
    vec<int> a{
        3,1,4,1,5,9,2,6,5,3,5,8};
    int lo = 0, hi = 100;
    vec<int32_t> rb(6);
    std::iota(rb.begin(), rb.end(), int32_t{0});
    for (size_t i = 0; i < 3; ++i)
        rb[i] = int32_t(i),
        rb[i + 3] = int32_t(9 + i);
    // compile check; skip runCore call
    (void)a;
    return 0;
}
