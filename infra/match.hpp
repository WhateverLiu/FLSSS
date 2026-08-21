#pragma once
#include "LookupViaHashingView.hpp"


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


//*
//* For each element `y[i]` in `[y, yend)`, get the index of an equal element
//* in `[x, xend)`. If there is no such element, the index is set to 
//* `xend - x`.
//* 
//* `rst[ ]` is a buffer of size at least `yend - y`.
//* 
//* The function is equivalent to R's `match()` except it does not
//* necessarily return the first qualified element in `[x, xend)`.
//* To ensure the firstness, do 
//* `HashmapView < ensureFirstOccurrenceSelected = true >`.
//* 
//* Speed is 5x faster than R's match().
//* 
void match( auto y, auto yend, auto x, auto xend, auto rst ) {
  auto shv = LookupViaHashingView(x, xend);
  parFor (0, yend - y, [&shv, x, rst, y](auto i)->void {
    rst[i] = shv.find(y[i]) - x;
  });
}


void match( auto y, auto yend, auto x, auto xend, auto rst, 
            auto && hs, auto && eq ) {
  auto shv = LookupViaHashingView(x, xend, hs, eq);
  parFor (0, yend - y, [&shv, x, rst, y](auto i)->void {
    rst[i] = shv.find(y[i]) - x;
  });
}


template <typename Int = uint32_t>
auto match (auto && y, auto && x) {
  vec<Int> rst(y.size());
  match(y.begin(), y.end(), x.begin(), x.end(), rst.begin());
  return rst;
}


template <typename Int = uint32_t>
auto match (auto && y, auto && x, auto && hs, auto && eq) {
  vec<Int> rst(y.size());
  match(y.begin(), y.end(), x.begin(), x.end(), rst.begin(), hs, eq);
  return rst;
}


#ifdef vecdef
#undef vec
#undef vecdef
#endif











