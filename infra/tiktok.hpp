#pragma once
#include <chrono>


#ifndef vec
#include <vector>
#define vec std::vector
#define vecNewlyDefined
#endif


/*
 // timetype =
 // std::chrono::hours,
 // std::chrono::minutes,
 // std::chrono::seconds,
 // std::chrono::milliseconds,
 // std::chrono::microseconds,
 // std::chrono::nanoseconds
 */
template<typename timetype = std::chrono::microseconds>
struct tiktok {
  vec<std::chrono::time_point<std::chrono::steady_clock> > starts;
  void reset() { starts.reserve(10); starts.resize(0); }
  tiktok() { reset(); }
  
  // Register timestamp.
  std::size_t tik() { 
    starts.emplace_back(std::chrono::steady_clock::now()); 
    return 0; 
  }
  
  // Return time passed since registration.
  std::size_t tok() {
    std::size_t rst = std::chrono::duration_cast<timetype> (
      std::chrono::steady_clock::now() - starts.back()).count();
    starts.pop_back();
    return rst;
  }
};




#ifdef vecNewlyDefined
#undef vec
#endif




