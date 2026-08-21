#pragma once
#include <iostream>
#include <cmath>


// namespace tp {
/*
template <int size = 100> // Number of items to print, e.g. 100.
struct ProgressBar // Suitable in multithreading environment.
{
  int64_t which;
  std::size_t gap;
  bool p[size];
  ProgressBar(std::size_t imax)
  { 
    std::fill(p, p + size, false); 
    gap = (imax + (size - 1)) / size;
  }
  int64_t operator()(std::size_t i)
  {
    which = i / gap;
    if (!p[which]) 
    {
      p[which] = true;
      return which;
    }
    return -1;
  }
};
*/


template <size_t size = 100>
struct PrintProgress {
  size_t step_1, Nstep, currentStep, imax;
  std::mutex mx;
  bool isWholeStep (size_t i) { return (i & step_1) == 0; }
  void reset(size_t imax) {
    auto stepTmp = std::max<double>(1, std::ceil(imax / double(size)));
    size_t stepPower = std::round(std::ceil(std::log2(stepTmp)));
    step_1 = (1 << stepPower) - 1;
    Nstep = (imax >> stepPower) + ((imax & step_1) != 0);
    this->imax = imax;
    currentStep = 0;
  }
  PrintProgress(size_t imax) { reset(imax); } 
  void operator()(size_t i, auto&& out) {
    if ( i + 1 != imax and !isWholeStep(i) ) return;
    mx.lock();
    out << "Progress: [";
    int k = 0;
    for (int kend = currentStep; k < kend; ++k) out << '=';
    for (int kend = Nstep; k < kend; ++k) out << ' ';
    out << "] ";
    size_t pg = std::round(currentStep / double(Nstep) * (100 * 10)  );
    // The space is needed to let '\r' fully cover the previous line.
    if (pg >= 1000) out << "100% \n"; // Do not let the last line end with '\r'. 
    else 
      out << std::to_string(pg / 10) + "." + std::to_string(pg % 10) + "%\r";
    currentStep += 1;
    mx.unlock();
  } 
};


// =============================================================================
// Code example:
// =============================================================================
// Charlie::ProgressBar<100> pb(Nevent - NcoreEvent);
// if (verbose) Rcout << "Progress %: ";
// auto f = [&](std::size_t i, std::size_t t)->bool // A function to be loaded to many threads.
// {
//   if (verbose and t == 0)
//   {
//     auto p = pb(i - NcoreEvent);
//     if (p != -1) Rcout << p << " ";
//   }
//   ...
//   return false;
// }




