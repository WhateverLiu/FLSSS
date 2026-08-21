#pragma once


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


/**
 * `indicator` declares the type of the subset. 'b' means the indicators
 * are boolean values. 'i' means they are indices.
 * `swap` declares the assignment operators. `false` means copy.
 */
template <char indicator = 'i', bool move = false, bool singleThread = false>
auto subset (auto && seq, auto && sub) {
  static_assert (indicator == 'i' or indicator == 'b', 
                 "Indicator can only be 'i', integer, or 'b', boolean.");
  vec<std::remove_reference_t<decltype(*seq.begin())>> rst;
  // std::remove_reference_t<decltype(seq)> rst;
  if constexpr (indicator == 'b') {
    myassert(sub.size() - seq.size() == 0, "sub.size() != seq.size()");
    if constexpr (singleThread) {
      rst.reserve(std::accumulate(sub.begin(), sub.end(), 0ull));
      for (size_t i = 0, iend = seq.size(); i < iend; ++i) {
        if (sub[i]) rst.emplace_back(seq[i]); 
      }
    } else { 
      map ( filter(iota(seq.size()), [&sub](auto i)->bool { return sub[i]; }),
            [&seq](auto && x)->auto { return seq[x]; }).swap(rst);
    }
  } else { 
    if constexpr (singleThread) {
      rst.reserve(sub.size());
      for (size_t i = 0, iend = sub.size(); i < iend; ++i) {
        if constexpr (!move) rst.emplace_back(seq[sub[i]]);
        else rst.emplace_back(std::move(seq[sub[i]]));
      }
    } 
    else { 
      if constexpr (!move) map (sub, [&seq](auto && x)->auto { 
        return seq[x]; }).swap(rst);
      else {
        map (sub, [&seq](auto && x)->auto { 
          return std::move(seq[x]); }).swap(rst);
      }
    }   
  }
  return rst;
} 


#ifdef vecdef
#undef vec
#undef vecdef
#endif



