#pragma once
#include "whichNotIn.hpp"


auto setdiff (auto && x, auto && y) {
  return remove_duplicates(subset(x, whichNotIn(x, y)));
}
