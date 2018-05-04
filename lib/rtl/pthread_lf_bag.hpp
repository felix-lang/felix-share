#line 1279 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __FLX_PTHREAD_LF_BAG_H__
#define __FLX_PTHREAD_LF_BAG_H__

#include "flx_pthread_config.hpp"
#include <stdint.h>
#include <atomic>
#include "pthread_thread_control_base.hpp"

namespace flx { namespace pthread {

struct PTHREAD_EXTERN pthread_lf_bag {
  ::std::atomic <void *> * volatile a;
  size_t n;
  thread_control_base_t *tc;

  // for statistics
  size_t throughput;

  // these indices are for optimisation purposes ONLY
  // the head points at the next element to dequeue or a bit earlier
  ::std::atomic<size_t> head;

  // we can't use unsigned type because the value may go negative
  // if dequeue operations decrement the counter before the enqueue
  // that pushed the data does.
  ::std::atomic<int32_t> used;

  pthread_lf_bag (thread_control_base_t *tc_, size_t n_);

  // the destructor is not safe!
  // to make it safe one needs to be sure the queue is empty
  // AND that no more values will be enqueued.
  // This is very hard to do. Using a smart ptr for the bag
  // ensures there will be no more enqueue operations started
  // but not that one is not in progress. The queue may appear
  // empty during the progress of such final enqueue operations.
  // there is no safe way to ensure the queue will remain empty.
  ~pthread_lf_bag();

  void enqueue(void *d);
  void *dequeue ();
};

}} // namespaces
#endif
