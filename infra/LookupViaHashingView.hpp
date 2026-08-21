#pragma once
#include <type_traits>
// #include "parlay/primitives.h"
// #include "myassert.hpp"


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


// Check if an object is a random access iterator.
template <typename T> concept isRAI = std::random_access_iterator<T>;


/**
 * Given a range of values `[x, xend)`, build a "static" hashmap. 
 * Querying the hashmap with value `y` will return the iterator pointing to
 * the same value within `[x, xend)`.
 * 
 * Compared to the hashmap that can be created using `parlay::group_by()`,
 * my implementation is about 1.15x faster. And the memory consumption
 * is substantially lower.
 * 
 * @tparam T  Random access iterator type, the type of `x` and `xend`.
 * 
 * @tparam lookupTableType  
 *   - If 'm', behave like a hashmap, assume the input iterator 
 *     points to a sequence of key-value pairs, assume the hash 
 *     function hashes keys, and the equal function compares keys. 
 *     Meanwhile, assume `find()` takes in a key type. Anyway, 
 *     use it like a `std::unordered_map`.
 *    
 *   - If 's', behave as a hashset, assume the input iterator
 *     points to a sequence of keys, assume the hash function 
 *     hashes keys, and the equal function compares keys.
 *     Anyway, use it like a `std::unordered_set`.
 * 
 * @tparam ensureFirstOccurrenceSelected  A boolean value. `false` will
 * speedup the hashmap's creation, while the iterator
 * resulted from querying `y` will not necessarily point to the first
 * equal instance within `[x, xend)`. `true` will guarantee otherwise.
 * 
 * @tparam buildWithMultithreading  Hashmap creation with multithreading.
 * 
 * @tparam Int  Integer type used to store offsets inside the hashmap. The 
 * type should be large enough to store `xend - x`. Will throw 
 * exception otherwise.
 * 
 * @tparam Hash  Type of the hash functor which instructs how to hash
 * the elements within `[x, xend)`. For example, if the range `[x, xend)`
 * stores key-value pairs, then `Hash` should be a functor that 
 * takes in a pair type and produces the hash of the key.
 * 
 * @tparam Equal  Type of the equal functor which determines if two 
 * instances of *T are equal. For example, if the range `[x, xend)`
 * stores key-value pairs, then `Equal` should be a functor that 
 * takes in two pairs and decides if the keys are equal.
 * 
 * @param x  Beginning iterator of the range `[x, xend)`.
 * 
 * @param x  Ending iterator of the range `[x, xend)`.
 * 
 * @param hf  Hash function instantiation.
 * 
 * @param eq  Equal function instantiation.
 * 
 */
template <
  typename T, 
  char lookupTableType = 's',
  bool ensureFirstOccurrenceSelected = false,
  bool buildWithMultithreading = true, 
  typename Int = uint32_t,
  typename Hash = std::hash<typename std::iterator_traits<T>::value_type>,
  typename Equal = std::equal_to<typename std::iterator_traits<T>::value_type>
>
class LookupViaHashingView {
public: 
  LookupViaHashingView(){}
  LookupViaHashingView( 
    T x, 
    T xend,
    Hash  hashFun_  = Hash(),
    Equal equalFun_ = Equal()
  ) {
    reset(x, xend, std::move(hashFun_), std::move(equalFun_));
  }
  
  
  void reset ( 
      T x, 
      T xend,
      Hash  && hashFun_ = Hash(),
      Equal && equalFun_ = Equal()
  ) {
    
    static_assert(!std::is_signed<Int>::value, 
                  "Integers for storing the indices should be unsigned.");
    
    myassert ( xend - x <= Int(0) - 1, "Integer size is not large enough.");
    
    // static_assert(isRAI<decltype(x)>, 
    //               "x is not a random access iteartor");
    // 
    // static_assert(isRAI<decltype(xend)>, 
    //               "xend is not a random access iteartor");
    
    hashFun = std::move(hashFun_);
    equalFun = std::move(equalFun_);
    this->x = x;
    this->xend = xend;
    Int xsize = xend - x;
    prime = lowestUBprime(xsize * 1.3);
    auto groupID = vec<Int>::uninitialized(xsize);
    auto GID = groupID.data();
    
    
    auto hf = [this](auto && x)->auto {
      if constexpr (lookupTableType == 's') return hashFun(x);
      else return hashFun(std::get<0>(x));
    };
    
    
    if constexpr (!buildWithMultithreading) {
      // for (auto it = x; it < xend; ++it)
      //   groupID.emplace_back(hf(*it) % prime);
      for (Int i = 0; i < xsize; ++i) // Not using *it to enable delayed sequence.
        GID[i] = hf(x[i]) % prime;  
    }
    else {
      parFor (
        0, xsize, [GID, x, this, &hf](auto i)->void { 
          GID[i] = hf(x[i]) % prime; });
    }
    
    
    groupBegin = vec<Int>::uninitialized(prime + 2);
    auto gb = groupBegin.data() + 2;
    if constexpr (!buildWithMultithreading) {
      std::fill(gb - 2, gb + prime, 0);
      for (auto i = GID, iend = GID + xsize; i < iend; ++i) gb[*i] += 1;
    }
    else {
      gb[-1] = gb[-2] = 0;
      auto hist = histogram_by_index ( // parlay::histogram_by_index() is 1.45x faster than atomic addition..
        slice(GID, GID + xsize), prime);
      copy(hist, slice(gb, gb + prime)); // This copy is still worth it..
    }

        
    if constexpr (!buildWithMultithreading)
      std::partial_sum(gb, gb + prime, gb);
    else {
      parlay::scan_inclusive_inplace(
        parlay::slice(gb, gb + prime)); // Be careful the vector size is 0.
    }
    
    
    bucket = vec<Int>::uninitialized(xsize);
    B = bucket.data();
    gb -= 1;
    
    
    // Turns out that it is quite worth it using a sequence of 
    // spinlocks when contentions are few.
    if constexpr (!buildWithMultithreading or ensureFirstOccurrenceSelected) {
      for (Int i = 0; i < xsize; ++i) {
        auto& p = gb[GID[i]];
        B[p] = i;
        ++p;
      }
    }
    else  { 
      vec<char> flags(prime, false);
      auto fl = (std::atomic_flag*)(flags.data()); 
      parlay::parallel_for(0, xsize, [&](auto i)->void {
        auto g = GID[i]; // Which group.
        while ( fl[g].test_and_set(std::memory_order_acquire) ); // Spinlock
        auto & p = gb[g];
        B[p] = i;
        ++p;
        fl[g].clear(std::memory_order_release);
      });
    }
  }
  
  
  auto begin() { return x; }
  
  
  auto end() { return xend; }
  
  
  auto find ( auto && y ) {
    auto eq = [this](auto && x, auto && y)->auto {
      if constexpr (lookupTableType == 's') return equalFun(x, y); // x and y are both keys.
      else return equalFun(std::get<0>(x), y); // y is key.
    };
    auto rst = xend;
    auto gid = hashFun(y) % prime;
    auto gb = groupBegin.data();
    // Do not parallelize this for() because uend - u is of O(1) on average.
    for (auto u = gb[gid], uend = gb[gid + 1]; u < uend; ++u) {
      if ( eq(x[B[u]], y) ) { rst = B[u] + x; break; }
    }
    return rst;
  }
  
  
private:
  T x, xend;
  size_t prime;
  
  
  // refrain from consolidating the 3 memory allocations into 1. 
  // The time saved would be negligible, and the readability
  // would be hampered. More importantly, it is more likely that the memory
  // pool already has buffers with similar sizes, and buffers newly allocated
  // in this function will be likely used later in the app.
  vec<Int> bucket, groupBegin;
  Int * B; // = bucket.data();
  Hash hashFun;
  Equal equalFun;
};




#ifdef vecdef
#undef vec
#undef vecdef
#endif







