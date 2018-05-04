#line 1436 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __FLX_PTHREAD_BOUND_QUEUE_H__
#define __FLX_PTHREAD_BOUND_QUEUE_H__
#include "flx_pthread_config.hpp"
#include "flx_gc.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

// interface for a consumer/producer queue. threads requesting a resource
// that isn't there block until one is available. push/pop re-entrant

namespace flx { namespace pthread {

// ********************************************************
/// Thread safe bounded queue.
///
/// The queue can be locked by setting bound=0.
/// In this state it can only be unlocked by setting a non-zero bound.
///
/// If the bound is set to 1 (the default),
/// then the queue is always either empty or full.
/// An empty queue blocks readers until a writer sends some data.
/// A full queue blocks writers, until a reader reads the data.
/// Note that when the queue is empty a writer can write data
/// and continues without waiting for the data to be read.
// ********************************************************

class PTHREAD_EXTERN bound_queue_t :public world_stop_notifier_t {
  thread_control_base_t *tc;
  ::std::condition_variable_any size_changed;
  ::std::mutex member_lock;
  size_t bound;
  void notify_world_stop() override;
  void wait();
  void wait_no_world_stop_check(); // used by async system
public:
  void *lame_opaque; // has to be public for the scanner to find it
  bound_queue_t(thread_control_base_t *tc_, size_t);
  ~bound_queue_t();
  void enqueue(void*);
  void enqueue_no_world_stop_check(void*); // used by async system
  void* dequeue();
  void* maybe_dequeue();
  void resize(size_t);
  void wait_until_empty();
  size_t len();
};

PTHREAD_EXTERN ::flx::gc::generic::scanner_t bound_queue_scanner;

}} // namespace pthread, flx
#endif

