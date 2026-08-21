#pragma once
#include "LookupViaHashingView.hpp"
#include "parlay/primitives.h"


/**
 * Given a range of key-value pairs, produce an object that allows 
 * constant time query of keys.
 * 
 * @tparam Key  Type of the key.
 * 
 * @tparam Val  Type of the value.
 * 
 * @tparam Int  Integer type for internal storage. Should be
 * able to store the total number of key-value pairs.
 * 
 * @tparam Pair  Key-value pair type.
 * 
 * @tparam Range  Container type for storing the pairs.
 * 
 * @tparam buildWithMultithreading  Boolean. Whether to build
 * the lookup table using multiple threads. See the same 
 * parameter in `LookupViaHashingView`.
 * 
 * @tparam Hash  Hash functor type. Will hash the key.
 * 
 * @tparam Equal  Equal functor type. Will compare keys.
 * 
 * Class has been fully tested. 
 * 
 */
template <
  typename Key, typename Val,
  typename Int = uint32_t, 
  template <typename...> typename Pair = std::pair,
  template <typename> typename Range = parlay::sequence,
  bool buildWithMultithreading = true,
  typename Hash = std::hash<Key>,
  typename Equal = std::equal_to<Key>
>
class GroupByHashmapView {  
public:
  GroupByHashmapView(){}
  GroupByHashmapView(
    Range<Pair<Key, Val> >&& rg, 
    Hash  hf = std::hash<Key>(),
    Equal eq = std::equal_to<Key>()
  ) {
    reset(std::move(rg), std::move(hf), std::move(eq) );
  }
  
  /**
   * @param rg  A range object.
   * 
   * @param hf  Hash functor instantiation.
   * 
   * @param eq  Equal functor instantiation.
   */
  void reset(
    Range<Pair<Key, Val> >&& rg,
    Hash&& hf = std::hash<Key>(),
    Equal&& eq = std::equal_to<Key>()
  ) {
    parlay::group_by_key(rg, hf, eq).swap(G);
    H.reset(&*G.begin(), &*G.end(), std::move(hf), std::move(eq));
  }
  auto begin() { return H.begin(); }
  auto end() { return H.end(); }
  auto find(const auto& key) { return H.find(key); }


private:  
  // ===========================================================================
  // Here, the type
  // cannot be the generic type `Pair` because `parlay::group_by_key()`
  // always returns a sequence of `std::pair`. So, if `Pair` is 
  // `std::tuple<Key, Val>`, which is the return type of `parlay::zip()`,
  // then using `Range < std::tuple <...> >` will get you error.
  // ===========================================================================
  Range < std::pair< Key, parlay::sequence<Val> > > G; 
  LookupViaHashingView < 
    std::pair<Key, parlay::sequence<Val> >*,
    'm', false, buildWithMultithreading, 
    Int, Hash, Equal> H;
};














