#pragma once
#include "LookupViaHashingView.hpp"


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


template <typename Int = uint32_t>
auto whichIn ( auto && y, auto && x) {
  myassert(x.size() <= Int(0) - 1, "x.size() > max value represented by Int." );
  auto shv = LookupViaHashingView(x.begin(), x.end());
  return filter ( iota(Int(y.size())), [&](auto i)->bool {
    return shv.find(y[i]) != x.end();
  });
}


/**
 * whichIn() but both inputs are sequences of integers. Additionally, 
 * the second sequence `x` shall only contain nonnegative integer.
 * `n` is the size of the boolean container, and satisfy 
 * `n >= x.max + 1` and `n >= y.max + 1`.
 */
template <typename Int>
auto whichIn ( auto && y, auto && x, Int n) {
  vec<bool> indi(n, false);
  for_each(x, [&](auto && u)->void { indi[u] = true; });
  return filter(iota(Int(y.size())), [&](auto i)->bool { return indi[y[i]]; });
}


#ifdef vecdef
#undef vec
#undef vecdef
#endif


