#pragma once
#include "LookupViaHashingView.hpp"


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


template <typename Int = uint32_t>
auto whichNotIn ( auto && y, auto && x ) {
  myassert(x.size() <= Int(0) - 1, "x.size() > max value represented by Int." );
  auto shv = LookupViaHashingView(x.begin(), x.end());
  return filter ( iota(Int(y.size())), [&](auto i)->bool {
    return shv.find(y[i]) == x.end();
  });
}


#ifdef vecdef
#undef vec
#undef vecdef
#endif


