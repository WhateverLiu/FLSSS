#pragma once
#include <algorithm>
#include <atomic>
#include <concepts>
#include <condition_variable>
#include <mutex>
#include <thread>

template<class P>
concept MsfdProblem =
    std::default_initializable<P> &&
    requires(P& a, P& b) {
        { a.split(b) } -> std::same_as<void>;
        { a.can_split() } -> std::same_as<bool>;
        { a.is_solved() } -> std::same_as<bool>;
        { a.a_step_forward() } ->
            std::same_as<void>;
        { a.swap(b) } -> std::same_as<void>;
        { a.merge(b) } -> std::same_as<void>;
    };

enum class MsfdPool {
    ParallelFor,
    Jthread
};

template<MsfdProblem P>
void msfd(P& root, int n_threads,
          bool forward_first,
          MsfdPool pool = MsfdPool::ParallelFor) {
    using enum std::memory_order;
    const int cap = pool == MsfdPool::ParallelFor
        ? std::max(1, int(num_workers()))
        : std::max(1, int(
            std::thread::hardware_concurrency()));
    int T = std::clamp(n_threads, 1, cap);

    if (forward_first) root.a_step_forward();
    if (root.is_solved()) return;
    if (T == 1) {
        while (!root.is_solved())
            root.a_step_forward();
        return;
    }

    P* W = new P[T];
    W[0].swap(root);

    std::mutex mu;
    std::condition_variable cv;
    alignas(64) std::atomic<int>
        n_idle{T - 1};
    int n_live = 1;

    // idle: parked, no work
    // ready: slot filled, wake and run
    // busy: solving
    // taken: reserved while donor splits
    enum : unsigned char {
        idle = 0, ready, busy, taken
    };
    auto* st =
        new std::atomic<unsigned char>[T];
    st[0].store(busy, relaxed);
    for (int i = 1; i < T; ++i)
        st[i].store(idle, relaxed);

    auto donate = [&](P& me) -> bool {
        if (n_idle.load(relaxed) <= 0)
            return false;
        if (!me.can_split()) return false;
        for (int y = 0; y < T; ++y) {
            unsigned char e = idle;
            if (!st[y].compare_exchange_strong(
                    e, taken, acq_rel, relaxed))
                continue;
            n_idle.fetch_sub(1, relaxed);
            me.split(W[y]);
            {
                std::lock_guard<std::mutex>
                    g(mu);
                st[y].store(ready, release);
                ++n_live;
            }
            cv.notify_all();
            return true;
        }
        return false;
    };

    auto run = [&](int x) {
        bool parked = x != 0;
        P& p = W[x];
        for (;;) {
            if (parked) {
                std::unique_lock<std::mutex>
                    lk(mu);
                cv.wait(lk, [&] {
                    return st[x].load(acquire)
                        == ready
                        || n_live == 0;
                });
                if (st[x].load(relaxed)
                    != ready)
                    return;
            }
            parked = true;
            st[x].store(busy, relaxed);
            while (!p.is_solved())
                if (!donate(p))
                    p.a_step_forward();
            int left;
            {
                std::lock_guard<std::mutex>
                    g(mu);
                root.merge(p);
                left = --n_live;
                st[x].store(idle, release);
                n_idle.fetch_add(1, relaxed);
            }
            if (left == 0) {
                cv.notify_all();
                return;
            }
        }
    };

    if (pool == MsfdPool::ParallelFor) {
        parallel_for(0, size_t(T),
            [&](size_t i) { run(int(i)); }, 1);
    } else {
        auto* ts = new std::jthread[T - 1];
        for (int i = 1; i < T; ++i)
            ts[i - 1] = std::jthread(
                [&run, i] { run(i); });
        run(0);
        delete[] ts;
    }
    delete[] st;
    delete[] W;
}

template<MsfdProblem P>
void MultithreadSolverForDecomposableProblems(
    P& problem, int n_threads,
    bool forward_first,
    MsfdPool pool = MsfdPool::ParallelFor) {
    msfd(problem, n_threads,
        forward_first, pool);
}
