

/**
 * `indicator` declares the type of the subset. 'b' means the indicators
 * are boolean values. 'i' means they are indices.
 * `swap` declares the assignment operators. `false` means copy.
 */
template <char indicator = 'i', bool singleThread = false, bool swap = false>
void subget (auto && seq, auto && sub, auto && assignment) {
  static_assert (indicator == 'i' or indicator == 'b', 
                 "Indicator can only be 'i', integer, or 'b', boolean.");
  if constexpr (indicator == 'b') {
    myassert(sub.size() - seq.size() == 0, "sub.size() != seq.size()");
    if constexpr (singleThread) {
      myassert(std::accumulate(sub.begin(), sub.end(), 0ull) - 
        assignment.size() == 0, "Sum of boolean indices != assignment.size()");
      for (size_t i = 0, iend = sub.size(), j = 0; i < iend; ++i) {
        if (sub[i]) { 
          if constexpr (swap) seq[i].swap(assignment[j]);
          else seq[i] = assignment[j]; 
          j += 1; 
        }
      }
    } else { 
      auto ind = filter(iota(seq.size()), [&sub](auto i)->bool { return sub[i]; });
      myassert(ind.size() - assignment.size() == 0, 
               "ind.size() != assignment.size()");
      parFor (0, ind.size(), [&](size_t i)->void {
        if constexpr (swap) seq[ind[i]].swap(assignment[i]);
        else seq[ind[i]] = assignment[i];
      });
    }
  } else { 
    myassert(sub.size() - assignment.size() == 0, 
             "sub.size() != assignment.size()");
    if constexpr (singleThread) {
      for (size_t i = 0, iend = sub.size(); i < iend; ++i) {
        if constexpr (swap) seq[sub[i]].swap(assignment[i]);
        else seq[sub[i]] = assignment[i];
      }
    } else {  
      parFor (0, sub.size(), [&](size_t i)->void {
        if constexpr (swap) seq[sub[i]].swap(assignment[i]);
        else seq[sub[i]] = assignment[i];
      });
    }  
  }
}  





