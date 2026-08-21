#pragma once
#include <random>


inline auto & sampleOne (auto && x, auto && rng) {
  return x[std::uniform_int_distribution<size_t> (0, x.size() - 1)(rng)];
}


