#pragma once
#include <random>


template <bool left2right = true, bool ignoreOrder = true>
void fisherYates (auto begin, auto end, size_t n, auto && rng) {
  size_t size = end - begin;
  n = std::min(n, size);
  std::uniform_real_distribution<double> U (0, 1);
  auto core = [&]<bool l2r>(size_t n) {
    for (size_t i = 0; i < n; ++i) {
      auto k = size_t(U(rng) * (size - i));
      if constexpr (l2r) std::swap(begin[i], begin[i + k]);
      else std::swap(begin[size - i - 1], begin[k]);
    }
  };
  if constexpr (ignoreOrder) 
    core.template operator () <left2right> (std::min(n, size - n));
  else 
    core.template operator () <left2right> (n);
} 








