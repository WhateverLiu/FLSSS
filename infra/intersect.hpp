#pragma once
#include "whichIn.hpp"


auto intersect (auto && x, auto && y) {
  if (x.size() < y.size() ) return intersect ( y, x );
  return remove_duplicates(subset(x, whichIn (x, y)));
}

