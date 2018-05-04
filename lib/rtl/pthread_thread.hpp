#line 54 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __FLX_PTHREAD_THREAD_H__
#define __FLX_PTHREAD_THREAD_H__
#include "flx_pthread_config.hpp"

#if FLX_WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif

// auto pthread, because I forget how to deallocate them nicely
// could init in the constructor, but ultimately you don't want the thread
// barging in before you've finished doing other stuff
// Addendum (20051128): doing stdio in turns out to be not very safe.
// I don't know if printf et al are supposed to be thread safe (most impls
// seem to try to be) but I sometimes get deadlocks in ppc64 os x 10.4.2
// with 4.0.1 when printfing to stdout. Nasty.

#include "pthread_thread_control_base.hpp"

#include <utility>
#include <map>
#include <vector>
#include <functional>
#include <thread>
#include <mutex>
#include <condition_variable>

namespace flx { namespace pthread {

// ********************************************************
/// Posix Threads. This class simply wraps the creation
/// and joining of threads. It is not safe.
// ********************************************************

#ifdef _WIN32
typedef HANDLE flx_native_thread_t;
#else
typedef pthread_t flx_native_thread_t;
#endif

flx_native_thread_t PTHREAD_EXTERN get_current_native_thread();
uintptr_t PTHREAD_EXTERN mythrid();

typedef std::pair<uintptr_t const, thread_data_t> thread_entry_t;
typedef std::map<uintptr_t, thread_data_t, std::less<uintptr_t> > thread_registry_t;

class PTHREAD_EXTERN thread_control_t : public virtual thread_control_base_t
{
    thread_control_t (thread_control_t const &) = delete; // uncopyable
    void operator=(thread_control_t const&) = delete; // uncopyable
    bool do_world_stop;
    size_t thread_counter;
    size_t active_counter;
    ::std::condition_variable_any stop_guard;
    ::std::mutex stop_mutex;
    thread_registry_t threads;
    void unsafe_stop_check();
    void unsafe_suspend();
    void unsafe_resume();

    world_stop_notifier_t **world_stop_notifier_array;
    size_t world_stop_notifier_array_length;
    void world_stop_notify();
public:
    bool debug;
    bool get_debug()const override;
    thread_control_t (bool);
    size_t thread_count() override;
    size_t active_count();
    void add_thread(void*) override;
    void remove_thread() override;
    bool world_stop() override;
    void join_all()  override;
    void world_start() override;
    void yield() override;
    void suspend() override;
    void resume() override;
    void register_world_stop_notifier(world_stop_notifier_t *) override;
    void unregister_world_stop_notifier(world_stop_notifier_t *) override;
    memory_ranges_t *get_block_list() override; // called owns result and should delete it
};

struct tstart_t
{
  void (*sr)(void*);
  void *cd;
  thread_control_base_t *tc;
  ::std::mutex *spawner_lock;
  ::std::condition_variable_any *spawner_cond;
  bool *spawner_flag;

  tstart_t(void (*s)(void*),void* c,thread_control_base_t *t, ::std::mutex *sl, ::std::condition_variable_any *sc, bool *sf)
    : sr(s), cd(c), tc(t), spawner_lock(sl), spawner_cond(sc), spawner_flag(sf)
  {}
};

// a class for threads that can't be joined. upon exit all their resources
// are freed. they just evaporate. probably the best type of thread.
class PTHREAD_EXTERN flx_detached_thread_t {
  flx_native_thread_t thr;        ///< the thread
  flx_detached_thread_t(flx_detached_thread_t const&); // uncopyable
  void operator=(flx_detached_thread_t const&); // uncopyable
public:
  flx_detached_thread_t();
  ~flx_detached_thread_t();
  int init(void (*start)(void*), void* udat, thread_control_base_t*, ::std::mutex *, ::std::condition_variable_any *, bool*);
};

// rf: joinable threads. is it an error to not join joinable threads?
class PTHREAD_EXTERN flx_thread_t {
  flx_native_thread_t thr;        ///< the thread
  flx_thread_t(flx_thread_t const&); // uncopyable
  void operator=(flx_thread_t const&); // uncopyable
public:
  flx_thread_t();
  ~flx_thread_t();
  int init(void (*start)(void*), void* udat, thread_control_base_t*);
  void join();
};

/// RAII wrapper for thread class
class PTHREAD_EXTERN flx_thread_wrapper_t {
  flx_thread_t thread;
  flx_thread_wrapper_t(flx_thread_wrapper_t const&); // uncopyable
  void operator=(flx_thread_wrapper_t const&); // uncopyable
public:
  ~flx_thread_wrapper_t();
  flx_thread_wrapper_t(void (*start)(void*), void* udat, thread_control_base_t *tc);
};

}}
#endif

