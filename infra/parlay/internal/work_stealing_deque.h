
#ifndef PARLAY_INTERNAL_WORK_STEALING_DEQUE_H_
#define PARLAY_INTERNAL_WORK_STEALING_DEQUE_H_

#include <cassert>

#include <atomic>
#include <iostream>
#include <utility>

namespace parlay {
namespace internal {

// Deque from Arora, Blumofe, and Plaxton (SPAA, 1998).
//
// Supports:
//
// push_bottom:   Only the owning thread may call this
// pop_bottom:    Only the owning thread may call this
// pop_top:       Non-owning threads may call this
//
template <typename Job>
struct Deque {
  using qidx = unsigned int;
  using tag_t = unsigned int;

  // use std::atomic<age_t> for atomic access.
  // Note: Explicit alignment specifier required
  // to ensure that Clang inlines atomic loads.
  struct alignas(int64_t) age_t {
    // cppcheck bug prevents it from seeing usage with braced initializer
    tag_t tag;                // cppcheck-suppress unusedStructMember
    qidx top;                 // cppcheck-suppress unusedStructMember
  };

  // align to avoid false sharing
  struct alignas(64) padded_job {
    std::atomic<Job*> job;
  };

  static constexpr int q_size = 1000;
  std::atomic<qidx> bot;
  std::atomic<age_t> age;
  std::array<padded_job, q_size> deq;

  Deque() : bot(0), age(age_t{0, 0}) {}

  // Adds a new job to the bottom of the queue if there is room. Only the
  // owning thread can push new items. This must not be called by any
  // other thread.
  //
  // Returns {was_empty, pushed}:
  //   pushed    - false iff the deque is full, in which case the job was NOT
  //               added and the caller must run it inline. This replaces the
  //               previous std::abort() on overflow, so deep nested
  //               parallelism now degrades gracefully to sequential execution
  //               instead of killing the process. See cmuparlay/parlaylib#99.
  //   was_empty - true if the queue was empty before a successful push
  //               (only meaningful when pushed == true).
  std::pair<bool, bool> push_bottom(Job* job) {
    auto local_bot = bot.load(std::memory_order_acquire);      // atomic load
    // Full check first, before touching any slot, so a failed push leaves the
    // deque completely untouched. Same effective capacity as before: this
    // branch simply replaces the old overflow abort() and adds no hot-path
    // work (it tests an already-loaded value).
    if (local_bot + 1 == q_size) {
      return {false, false};
    }
    deq[local_bot].job.store(job, std::memory_order_release);  // shared store
    local_bot += 1;
    bot.store(local_bot, std::memory_order_seq_cst);  // shared store
    return {(local_bot == 1), true};
  }

  // Pop an item from the top of the queue, i.e., the end that is not
  // pushed onto. Threads other than the owner can use this function.
  //
  // Returns {job, empty}, where empty is true if job was the
  // only job on the queue, i.e., the queue is now empty
  std::pair<Job*, bool> pop_top() {
    auto old_age = age.load(std::memory_order_acquire);    // atomic load
    auto local_bot = bot.load(std::memory_order_acquire);  // atomic load
    if (local_bot > old_age.top) {
      auto job = deq[old_age.top].job.load(std::memory_order_acquire);  // atomic load
      auto new_age = old_age;
      new_age.top = new_age.top + 1;
      if (age.compare_exchange_strong(old_age, new_age))
        return {job, (local_bot == old_age.top + 1)};
      else
        return {nullptr, (local_bot == old_age.top + 1)};
    }
    return {nullptr, true};
  }

  // Pop an item from the bottom of the queue. Only the owning
  // thread can pop from this end. This must not be called by any
  // other thread.
  Job* pop_bottom() {
    Job* result = nullptr;
    auto local_bot = bot.load(std::memory_order_acquire);  // atomic load
    if (local_bot != 0) {
      local_bot--;
      bot.store(local_bot, std::memory_order_release);  // shared store
      std::atomic_thread_fence(std::memory_order_seq_cst);
      auto job =
          deq[local_bot].job.load(std::memory_order_acquire);  // atomic load
      auto old_age = age.load(std::memory_order_acquire);      // atomic load
      if (local_bot > old_age.top)
        result = job;
      else {
        bot.store(0, std::memory_order_release);  // shared store
        auto new_age = age_t{old_age.tag + 1, 0};
        if ((local_bot == old_age.top) &&
            age.compare_exchange_strong(old_age, new_age))
          result = job;
        else {
          age.store(new_age, std::memory_order_seq_cst);  // shared store
          result = nullptr;
        }
      }
    }
    return result;
  }
};

}  // namespace internal
}  // namespace parlay

#endif  // PARLAY_INTERNAL_WORK_STEALING_DEQUE_H_
