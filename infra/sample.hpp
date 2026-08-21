#pragma once
#include "fisherYates.hpp"


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


template <bool replace = false, bool ignoreOrder = true, bool move = false>
auto sample (auto && x, size_t n, auto && rng) {
  using T = std::remove_reference_t<decltype(*x.begin())>;
  vec<T> rst; 
  if constexpr (replace) {
    myassert(n == 0 or x.size() > 0, "Cannot sample from an empty sequence.");
    rst.reserve(n);
    auto U = std::uniform_int_distribution<size_t> (0, x.size() - 1);
    for (size_t i = 0; i < n; ++i) {
      if constexpr (move) rst.emplace_back(std::move(x[U(rng)]));
      else rst.emplace_back(x[U(rng)]);
    }
  } 
  else {
    n = std::min(n, x.size());
    if constexpr (move) {
      rst.reserve(x.size());
      for (auto & u: x) rst.emplace_back(std::move(u));
    } else rst.assign(x.begin(), x.end());
    fisherYates<true, ignoreOrder>(rst.begin(), rst.end(), n, rng);
    rst.resize(n);
  }
  return rst;
}


#ifdef vecdef
#undef vec
#undef vecdef
#endif


