#pragma once
#include <algorithm>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <execution>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <thread>

#include "myassert.hpp"
#include "parlay/primitives.h"

namespace infra = parlay;


namespace parlay {

inline auto tid() { return worker_id(); }

template <typename... Args>
inline auto parFor(Args&&... args) -> decltype (
    parallel_for(std::forward<Args>(args)...)) {
  return parallel_for(std::forward<Args>(args)...);
}

template <typename... Args> // Shorthand for parlay::tabulate
inline auto tab(Args&&... args) -> decltype (
    tabulate(std::forward<Args>(args)...)) {
  return tabulate(std::forward<Args>(args)...);
}

template <typename... Args>  // Shorthand for parlay::delayed_tabulate
inline auto detab(Args&&... args) -> decltype (
    delayed_tabulate(std::forward<Args>(args)...)) {
  return delayed_tabulate(std::forward<Args>(args)...);
}


template <typename... Args>  // Shorthand for parlay::delayed_tabulate
inline auto demap(Args&&... args) -> decltype (
    delayed_map(std::forward<Args>(args)...)) {
  return delayed_map(std::forward<Args>(args)...);
}


// Zip two sequences as a sequence of pairs, not tuples.
inline auto pzip (auto && x, auto && y)->auto {
  return map(iota(std::min(x.size(), y.size())), [&](auto i)->auto {
    return std::pair(x[i], y[i]);
  });
}


// Delayed version: zip two sequences as a sequence of pairs, not tuples.
inline auto depzip (auto && x, auto && y)->auto {
  return demap(iota(std::min(x.size(), y.size())), [&](auto i)->auto {
    return std::pair(x[i], y[i]);
  });
}
}


namespace parlay {
#include "MiniPCG.hpp"
#include "mmhash.hpp"
#include "PrintProgress.hpp"


struct IntRAiter { // Integer random access iterator.
  std::size_t i; 
  void reset(std::size_t i) { this->i = i; }
  IntRAiter (std::size_t i) { reset(i); }
  using iterator_category = std::random_access_iterator_tag;
  using value_type = std::size_t;
  using difference_type = std::ptrdiff_t;
  using pointer = value_type*;
  using reference = value_type&;
  reference operator*() { return i; }
  pointer operator->() { return &i; }
  IntRAiter& operator++() { ++i; return *this; }
  IntRAiter operator++(int) { IntRAiter tmp = *this; ++(*this); return tmp; }
  IntRAiter& operator--() { --i; return *this; }
  IntRAiter operator--(int) { IntRAiter tmp = *this; --(*this); return tmp; }
  IntRAiter& operator+=(difference_type diff) { i += diff; return *this; }
  IntRAiter& operator-=(difference_type diff) { i -= diff; return *this; }
  difference_type operator-(const IntRAiter& other) const { return i - other.i; }
  IntRAiter operator+(difference_type diff) const { return IntRAiter(i + diff); }
  IntRAiter operator-(difference_type diff) const { return IntRAiter(i - diff); }
  bool operator==(const IntRAiter& other) const { return i == other.i; }
  bool operator!=(const IntRAiter& other) const { return i != other.i; }
  bool operator<(const IntRAiter& other) const { return i < other.i; }
  bool operator>(const IntRAiter& other) const { return i > other.i; }
  bool operator<=(const IntRAiter& other) const { return i <= other.i; }
  bool operator>=(const IntRAiter& other) const { return i >= other.i; }
  std::size_t operator[] (difference_type offset) const { return i + offset; }
};
using iit = IntRAiter;




#include "tiktok.hpp"
tiktok timer;
#include "lowestUBprime.hpp"
#include "match.hpp"
#include "GroupByHashmapView.hpp"
#include "isFinite.hpp"
#include "which.hpp"
#include "subset.hpp"
#include "subget.hpp"
#include "whichIn.hpp"
#include "whichNotIn.hpp"
#include "currentDateTime.hpp"
#include "logger.hpp"
#include "round.hpp"
#include "intersect.hpp"
#include "Union.hpp"
#include "setdiff.hpp"
#include "sample.hpp"
#include "static_loop.hpp"
#include "msfd.hpp"
}







