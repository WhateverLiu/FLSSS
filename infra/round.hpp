#pragma once
#include <cmath>
#include <cstddef>

template <int n = 2>
double round (double x) {
  size_t r = 1;
  for (int i = 0; i < n; ++i) r *= 10;
  return std::round(x * r) / r;
}

