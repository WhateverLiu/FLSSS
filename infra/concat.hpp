#pragma once
#include <type_traits>
#include <utility>


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


template <bool singleThread = false, bool move = false>
auto concat (auto && x, auto && y) {
  using T = std::remove_cvref_t<decltype(*x.begin())>;
  auto xsize = x.size();
  auto ysize = y.size();
  vec<T> rst; rst.reserve(xsize + ysize);
  if constexpr (singleThread) {
    if constexpr (move) {
      for (auto & u: x) rst.emplace_back(std::move(u));
      for (auto & u: y) rst.emplace_back(std::move(u));
    } else {
      for (auto && u: x) rst.emplace_back(u);
      for (auto && u: y) rst.emplace_back(u);
    }
  } else {
    rst = vec<T>::from_function(xsize + ysize, [&](size_t i) -> T {
      if (i < xsize) {
        if constexpr (move) return std::move(x[i]);
        else return x[i];
      } else {
        if constexpr (move) return std::move(y[i - xsize]);
        else return y[i - xsize];
      }
    });
  }
  return rst;
}


#ifdef vecdef
#undef vec
#undef vecdef
#endif
