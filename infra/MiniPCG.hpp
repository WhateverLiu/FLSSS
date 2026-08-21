#pragma once
#include <cstdint>
#include <string> // std::hash

struct MiniPCG {
  typedef uint32_t result_type;
  std::uint64_t state, multiplier, increment;
  std::uint32_t rotr32(std::uint32_t x, unsigned r) {
    return x >> r | x << (-r & 31u);
  }
  constexpr const static std::uint32_t min() { return 0ul; }
  constexpr const static std::uint32_t max() { return 4294967295ul; }
  std::uint32_t operator()() { 
    std::uint64_t x = state;
    unsigned count = (unsigned)(x >> 59u);		// 59 = 64 - 5
    state = x * multiplier + increment;
    x ^= x >> 18u;								// 18 = (64 - 27)/2
    return rotr32((std::uint32_t)(x >> 27u), count);	// 27 = 32 - 5
  }
  
  void reset() {
    state = 0x4d595df4d0f33173;
    multiplier = 6364136223846793005ull;
    increment  = 1442695040888963407ull;
  }
  
  void seed(std::uint64_t seed) { 
    reset();
    state = seed + increment;
    (*this)();
  }
  
  MiniPCG() { reset(); }
  MiniPCG(std::uint64_t s){ seed(s); }
  
};

