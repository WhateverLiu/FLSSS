#pragma once
#include <exception>


#define mydebug // Comment this line to disable myassert();
inline void myassert(bool b, auto && msg) {
#ifdef mydebug
  if (!b) throw std::runtime_error(std::string("Assert failed: ") + msg);
#endif
}




