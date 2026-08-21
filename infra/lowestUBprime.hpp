#pragma once
#include "modPrimes.hpp"
uint64_t lowestUBprime ( uint64_t x ) {
  auto it = std::lower_bound(modPrimes, modPrimes + 62, x);
  if (it != modPrimes + 62) return *it;
  return uint64_t(0) - 1;
}
