# Parlay: Windows deadlock at shutdown (scheduler join under the loader lock)

## Summary

On Windows, Parlay hangs at process exit when built into a DLL. The scheduler
joins its worker threads from a `thread_local` destructor. That join runs under
the loader lock during DLL detach. It deadlocks. Linux and macOS are fine.

## Where

- `scheduler.h` — `~scheduler()` calls `shutdown()`, which `join()`s every worker.
- `parallel.h` — `get_current_scheduler()` holds the scheduler as a
  `static thread_local`. It is destroyed at thread/process teardown.
- `internal/sequence_base.h` — `sequence_base()` calls `num_workers()` to
  "force the scheduler to initialize". So constructing any `parlay::sequence`
  spawns the worker threads, even in sequential code.

## When

All three must hold:

1. Platform is Windows. MSVC and MinGW both reproduce.
2. Parlay is in a DLL that gets unloaded at exit (e.g. a Python extension). A
   static `.exe` usually does not hang.
3. The scheduler was created: `maxCore() > 1` and some code reached it.
   Constructing a `parlay::sequence` is enough, so even sequential workloads
   trigger it.

`PARLAY_NUM_THREADS=1` spawns zero workers and exits cleanly. Use it to confirm,
not to fix.

## Why

The scheduler spawns persistent workers and parks them. Its instance is a
`static thread_local`, so its destructor runs at teardown and joins the workers.
On Windows, that teardown happens under the loader lock. `join()` waits for each
worker to exit, but an exiting thread needs the same loader lock for its own TLS
cleanup. That is a lock cycle. It deadlocks. This is the "never wait on threads
under the loader lock / in `DllMain`" rule.

## Reproduce

```python
import mymodule          # extension linking parlay
mymodule.run_something()  # returns fine; creates the scheduler
# process hangs on shutdown; must be killed
```

`PARLAY_NUM_THREADS=1` makes the same program exit instantly.

## Fix

The bug is joining threads at loader time. Preferred fix: never destruct the
process-lifetime scheduler on Windows. Then `shutdown()`/`join()` never run at
detach. The OS reclaims the parked workers at exit.

```cpp
// parallel.h, internal::get_current_scheduler()
if (current_scheduler == nullptr) {
#ifdef _WIN32
  // Never destruct on Windows: ~scheduler() joins workers, and that join runs
  // under the loader lock at DLL detach, which deadlocks. Leaking the singleton
  // skips the join; the OS reclaims the parked workers at process exit.
  static thread_local scheduler_type* local_scheduler =
      new scheduler_type(init_num_workers());
  return *local_scheduler;
#else
  static thread_local scheduler_type local_scheduler(init_num_workers());
  return local_scheduler;
#endif
}
```

One line. Windows-only. No runtime cost. Parallelism unchanged. Cost: a one-time
singleton leak.

Alternative (cleaner, more work): expose an explicit `destroy_scheduler()` and
call it before unload, while the loader lock is not held — for Python, via
`Py_AtExit`. This joins cleanly with no leak.

Do not detach the workers instead of joining. They would touch unmapped code
after unload and crash.

## Ask

Make "never destruct the process-lifetime scheduler" the Windows default, and/or
add an explicit `destroy_scheduler()` so DLL embedders can tear down safely
before unload.
