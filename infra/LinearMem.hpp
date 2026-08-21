/**
 * Uses the local Parlay scheduler indices to keep separate memory arenas per
 * worker.
 */
#pragma once
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <memory> // std::allocator_traits.
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#include "infra.hpp"


namespace LinearMem {
constexpr const size_t maxAlignByte = alignof(std::max_align_t);

struct Bucket {
  // maxEnd is used to how far the end pointer can maximally go.
  size_t *begin, *end, *cap, *maxEnd;
  void reserve(size_t N) {
    if (N == 0) [[unlikely]] return;
    N = std::max<size_t>(3, N);
    begin = new size_t [N];
    if ( (size_t(begin) & (maxAlignByte - 1) ) != 0 ) [[unlikely]] 
      throw std::runtime_error(
          "Operator new did not allocate alignof(std::max_align_t) "
          "aligned memory ?!");
    maxEnd = end = begin;
    cap = begin + N;
    // =========================================================================
    // Interestingly, uninitialized memory will not show up on Linux htop.
    //   Only when the memory is written will it be shown.
    // std::fill(begin, cap, size_t(0-1) );
    // =========================================================================
  }
  Bucket(size_t N) { reserve(N); }
  Bucket () { begin = end = cap = maxEnd = nullptr; }
  
  // ===========================================================================
  // ~Bucket () { delete [] begin; begin = end = cap = nullptr; }
  //   Omit destructor so that std::vector::pop_back() will not deallocate
  //   the memory.
  // ===========================================================================
};

inline size_t bucketMinSize_t = 0;
inline size_t workerBucketMinSize_t = 0;

struct Arena {
  size_t maxSize_tAlloced;
  std::vector<Bucket>* Vptr; // Typically points to Vprimary
  std::vector<Bucket> Vprimary;
  std::vector<Bucket> Vsecondary; // Rarely used for linearization.
  Arena() { Vptr = nullptr; maxSize_tAlloced = 0; }
};
inline std::vector<Arena> arenas; 

// =============================================================================
// It has been tested. Using spin lock is actually slower than using mutex.
//   By running random allocations, mutex gives
//   user.self   sys.self    elapsed user.child  sys.child 
//   8.164033   1.137467   0.825000   0.000000   0.000000
// while spin lock gives:
//   user.self   sys.self    elapsed user.child  sys.child 
//   12.6423000  0.3629333  0.8507333  0.0000000  0.0000000
// Even in single threaded environment, mutex seems advantagerous.
// =============================================================================

inline bool isReleased(size_t u) {
  constexpr const size_t dealloced = size_t(1) << (sizeof(size_t) * 8 - 1);
  return (u & dealloced) != 0;
}

inline void setRelease(size_t & u) {
  constexpr const size_t dealloced = size_t(1) << (sizeof(size_t) * 8 - 1);
  u |= dealloced;
}

inline size_t extractSize(size_t u) {
  constexpr const size_t dealloced_1 =
      (size_t(1) << (sizeof(size_t) * 8 - 1)) - 1;
  return u & dealloced_1;
}

inline size_t currentThreadIndex() {
  return parlay::tid();
}

inline int maxCore() {
  return std::max<int>(1, parlay::maxCore());
}

void stackRetreat(auto & V) {
  while (true) { 
    if (V.back().end == V.back().begin) { 
      if (V.size() == 1) break;
      V.pop_back();
    } 
    else { 
      if ( !isReleased( V.back().end[-1] ) ) break;
      V.back().end -= extractSize( V.back().end[-1] );
    }
  }
}

template <typename T>
struct Pool {
  using value_type = T;
  using pointer = T*;
  using const_pointer = const T*;
  using reference = T&;
  using const_reference = const T&;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;
  constexpr Pool() = default;
  template <typename U> 
  constexpr Pool ( const Pool <U> & ) noexcept {}
  
  
  // Do not delete. Some containers might still needed construct, rebind
  //   specified because their source code are in older C++ standard.
  /**
   * Here the new operator are placement new. It will not call `malloc` or `new`
   * if U is default constructible.
   */
  
  /*
  template <typename U, typename... Args>
  void construct(U* ptr, Args&&... args) {
    if constexpr (std::is_nothrow_default_constructible<U>::value and 
                    sizeof...(args) == 0) 
      ::new(static_cast<void*>(ptr)) U;
    else 
      ::new(static_cast<void*>(ptr)) U(std::forward<Args>(args)...);
  } 
  */
  
  /*
  template <typename U, typename... Args>
  void destroy(U* ptr, Args&&... args) {
    ptr->~U(std::forward<Args>(args)...);
  }  
  
  
  template <typename U>
  struct rebind { typedef Pool<U> other; };
   */
  
  
  size_t allocSize_t (size_t Nbyte) {
    constexpr const size_t addum = sizeof(size_t) + maxAlignByte - 1;
    return (Nbyte + addum) / maxAlignByte * (maxAlignByte / sizeof(size_t));
  }
  
  /**
   * Firstly, have the stack pointer retreat if it can. Secondly, check if the
   * current bucket can hold the new data.
   */
  T* allocate(size_t nT) {  
    auto N = allocSize_t( nT * sizeof(T) );
    size_t* rst = nullptr;
    auto tid = currentThreadIndex();
    auto bucketInitWords = tid == 0 ? bucketMinSize_t : workerBucketMinSize_t;
    auto& V = *arenas[tid].Vptr;
    stackRetreat(V); // Let the stack pointer retreat if it can.
    if ( V.back().end + N > V.back().cap ) { // Need a new bucket.
      // =====================================================================
      // If there are no variables in stack, and if
      //   the 1st bucket is already too small, just delete it and
      //   allocate a new, larger one.
      // =====================================================================
      if (V.size() == 1 and V.back().end == V.back().begin) {
        delete [] V.back().begin;
        V.back().reserve(N);
      }
      else {
        // ===================================================================
        // If capacity is reached, do not just emplace_back() because those
        //   extra buckets newly allocated would not have nullptr pointers.
        //   During the destruction of the pool, we loop through 
        //   [0, capacity()) to delete the array. This is why we manually
        //   double the size of V and set null pointers.
        // ===================================================================
        if (V.size() == V.capacity()) {
          V.resize( V.size() * 2, Bucket() );
          V.resize( V.size() / 2 );
          V.emplace_back( Bucket( std::max(N, bucketInitWords) ) );
        }
        // ===================================================================
        // If capacity is not reached, check if the next bucket already had
        //   been assigned with new[]. If yes, emplace_back() exactly the
        //   same bucket. Otherwise emplace_back() a newly initialized 
        //   bucket.
        // ===================================================================
        else {
          auto& nx = *(V.data() + V.size());
          if ( nx.begin != nullptr ) { // If a bucket already exists. 
            V.emplace_back( nx );
            // If the existing bucket is too small.
            if ( V.back().begin + N > V.back().cap ) {
              delete [] V.back().begin;
              V.pop_back();
              V.emplace_back ( Bucket( N ) );
            }
          }
          else V.emplace_back ( Bucket( std::max(N, bucketInitWords) ) );
        } 
      }
      V.back().end = V.back().begin;
    }
    
    rst = V.back().end;
    V.back().end += N;
    V.back().end[-1] = N; 
    arenas[tid].maxSize_tAlloced += N;
    V.back().maxEnd = std::max(V.back().maxEnd, V.back().end);
    return static_cast<T*>(static_cast<void*>(rst));
  } 
  
  
  /**
   * Just mark the deallocated block. We do not know if the block was allocated
   * by the current thread, so we place stackRetreat() in allocate().
   */
  void deallocate( T* t, size_t nT ) noexcept {
    // if (size_t(t) % 16 != 0)
    //   std::cout << "deallocate, What?! not 16-byte aligned!";
    if (nT == 0) [[unlikely]] return;
    auto x = static_cast<size_t*>(static_cast<void*>(t));
    size_t N = allocSize_t(nT * sizeof(T));
    setRelease( *(x + N - 1) );
  }
  
  
};

template <typename S, typename U>
constexpr bool operator == (const Pool<S> &t, const Pool<U> &u) {
  return true;
}

template <typename S, typename U>
constexpr bool operator != (const Pool<S> &t, const Pool<U> &u) {
  return false;
}

struct activate { 
  bool verbose;
  activate(activate const&) = delete;
  void operator=(activate const&) = delete;
  
  
  /**
   * Activate the memory pool.
   * 
   * @tparam bucketSizeUnit  A character in `{'B', 'K', 'M', 'G'}`, i.e. 
   * {byte, KB, MB, GB}.
   * 
   * @tparam verbose  True if the memory allocator should print usage
   * information before its destruction.
   * 
   * @param maxCore  Number of threads to be invoked. `maxCore > 1`
   * will trigger the construction of the thread pool, and activate the
   * mutex guard for allocation and deallocation.
   * 
   * @param bucketSize  How many `bucketSizeUnit`s should be allocated for
   * each bucket.
   */
  activate(
    bool verbose,
    size_t bucketSize, // MB
    size_t workerBucketSize, // MB
    char bucketSizeUnit) 
  {
    auto maxCore = LinearMem::maxCore();
    this->verbose = verbose;
    if (!(bucketSizeUnit == 'B' or bucketSizeUnit == 'K' or 
          bucketSizeUnit == 'M' or bucketSizeUnit == 'G'))
      throw std::runtime_error(
          "Bucket size unit is not one of "
          "{'B', 'K', 'M', 'G'}, i.e. "
          "byte, KB, MB, GB");
    
    // bucketMinSize_t: space measured in number of words.
    size_t scaler = 1;
    if (bucketSizeUnit == 'K') scaler = 1000;
    else if (bucketSizeUnit == 'M') scaler = 1000000;
    else if (bucketSizeUnit == 'G') scaler = 1000000000;
    bucketMinSize_t = std::max<size_t>(
      1, bucketSize * scaler / 8);
    workerBucketMinSize_t = std::max<size_t>(
      1, workerBucketSize * scaler / 8);
    
    // =========================================================================
    // If thread pool is not activated, use empty lock.
    // =========================================================================
    arenas.resize(maxCore);
    for (int a = 0; a < maxCore; ++a) {
      arenas[a].Vptr = &arenas[a].Vprimary;
      auto& V = *arenas[a].Vptr;
      // Do not use .reserve(). Need all the buckets have nullptr
      //   initializations.
      V.resize(5, Bucket());
      V.resize(0);
      // =======================================================================
      // For the main thread, preallocate memory of size `bucketMinSize_t`.
      //   For the worker threads, preallocate memory of size 
      //   `bucketMinSize_t / (maxCore - 1)`
      // =======================================================================
      // auto bsize = a == 0 ? bucketMinSize_t : workerBucketMinSize_t;
      V.emplace_back( Bucket( 1 ) );
      arenas[a].maxSize_tAlloced = 0;
    }
  }
  
  ~activate() {
    int mxc = arenas.size();
    size_t peakMemUsage = 0, maxSize_tAlloced = 0;
    std::vector<int> NbucketUsed;  
    if (verbose) NbucketUsed.reserve(mxc);
    for (int a = 0, aend = mxc; a < aend; ++a) {
      auto & V = *arenas[a].Vptr;
      size_t i = 0;
      size_t z = 0;
      for (size_t iend = V.capacity();
           i < iend and V[i].begin != nullptr; ++i) {
        z = V[i].maxEnd - V[i].begin;
        if (z > 3) peakMemUsage += V[i].maxEnd - V[i].begin;
        delete [] V[i].begin; 
      }
      maxSize_tAlloced += arenas[a].maxSize_tAlloced;
      if (i == 1 and z <= 3) NbucketUsed.emplace_back(0);
      else NbucketUsed.emplace_back(i);
    }
    
    if (verbose) {
      std::cout << "Peak memory usage = " <<
        peakMemUsage * 8.0 / (1000 * 1000) << " MB.\n";
      std::cout <<
        "Total size of all objects allocated throughout the run = " <<
        maxSize_tAlloced * 8.0 / (1000 * 1000) << " MB.\n";
      for (int i = 0, iend = NbucketUsed.size(); i < iend; ++i)
        std::cout << "Thread " << i << " space had "
                  << NbucketUsed[i] << " buckets.\n";
    }
    decltype(arenas)().swap(arenas);
  }
};


/**
 * Relocate objects of interest to linearize their memory layout.
 * 
 * The steps are as follows:
 * 
 *   -# Copy the objects to the secondary stack memory 
 *   space, the copied objects will have a compact, linear memory layout. 
 *   
 *   -# Remove the objects in the primary stack space. This is done by using 
 *   `std::move` to move the resources into temporary objects, which will 
 *   be auto destructed when going out of scope. 
 *   
 *   -# Copy resources from the secondary stack to the primary stack. 
 *   
 * Between the memory blocks that belong to any two objects in `...objs`, 
 * if there exists a block held by an in-use alien object, the original 
 * space allocated for `...objs` cannot be released. This is 
 * due to the memory's linear nature. Otherwise the original 
 * memory will be reused to store the linearized objects.
 * 
 * The initial motivation behind this function is to linearize the 
 * entire memory stack and remove "holes". This is done by
 * supplying the function all the objects that need to be saved.
 * 
 * All pointers and iterators associated with the original objects
 * are invalidated.
 * 
 * The function works even if the objects are on another thread's memory space.
 * 
 * The only requirements for `objs` are that they have empty constructors.
 *
 */
template <typename ... Ts>
void linearize ( Ts& ... objs )
{
  if (arenas.size() == 0) return;
  if ( sizeof...(objs) == 0 ) [[unlikely]] return;
  auto tid = currentThreadIndex();
  auto& Vptr = arenas[tid].Vptr;
  auto& Vprimary = arenas[tid].Vprimary;
  auto& Vsecondary = arenas[tid].Vsecondary;
  if (Vptr == nullptr or Vptr->size() == 0) return;
  
  // ===========================================================================
  // Change to the secondary stack which the allocator will be directed to.
  // ===========================================================================
  // Do not use .reserve. This is to ensure null pointers are properly
  //   initialized.
  Vsecondary.resize(Vprimary.size(), Bucket());
  Vsecondary.resize(0);
  Vsecondary.emplace_back( Bucket( 
      tid == 0 ? bucketMinSize_t : workerBucketMinSize_t ) );
  
  // ===========================================================================
  // First copy the object to the secondary space. Then destruct the object
  //   in the primary space. Finally copy the object in the secondary space
  //   to the primary space.
  // ===========================================================================
  ([&] { 
    Vptr = &Vsecondary;
    using objType = std::remove_reference<decltype(objs)>::type;
    objType objCopy = objs;
    Vptr = &Vprimary;
    { objType objsTmp = std::move(objs); }
    objs = objCopy;
    // objs.~objType(); // Deallocate all heap memory.
    // // =====================================================================
    // // Zero all the bits of objs on the stack to avoid error from
    // //   the 2nd time of calling the destructor when `objs` goes out of
    // //   scope.
    // // =====================================================================
    // std::memset((void*)&objs, 0, sizeof(objs)); 
    // objs = objCopy;
  } (), ...);
  
  for (auto & x: Vsecondary) delete [] x.begin; 
  std::remove_reference<decltype(Vsecondary)>::type ().swap(Vsecondary);
}
}

