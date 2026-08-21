#pragma once


#ifndef vec
#define vec parlay::sequence
#define vecdef
#endif


template< typename Int = uint32_t, bool singleThread = false>
auto which (auto && v, auto && bf)
{
  static_assert (std::is_unsigned_v<Int>, "Int shall be unsigned.");
  myassert(size_t(v.size()) <= size_t(Int(0) - 1), 
           "v.size() exceeds the max value in type Int.");
  vec<Int> rst;
  if constexpr (singleThread) {
    Int size = 0; for (auto && x: v) size += bf(x);
    rst.reserve(size);
    for (Int i = 0, iend = v.size(); i < iend; ++i) { 
      if (bf(v[i])) rst.emplace_back(i); 
    }
  }
  else filter(iota(Int(v.size())), [&](Int i)->bool { 
    return bf(v[i]); }).swap(rst);
  return rst;
}


#ifdef vecdef
#undef vec
#undef vecdef
#endif
