#line 1327 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
// simple very efficient lock free bag
#include <atomic>
#include <chrono>
#include <algorithm>
#include <thread>
#include <stdlib.h>
#include "pthread_lf_bag.hpp"
#include <assert.h>

using namespace flx::pthread;

// 10 ms max sleep, that's 10,000,000 nanoseconds
#define MAXSLEEP (size_t)10000000

static void sleep(thread_control_base_t *tc, size_t ns)
{
  assert(tc);
  tc->yield();
  //::std::this_thread::sleep_for(::std::chrono::nanoseconds(ns));
  ::std::this_thread::yield();
}

#define NQFENCE ::std::memory_order_seq_cst
#define DQFENCE ::std::memory_order_seq_cst


  pthread_lf_bag::pthread_lf_bag (thread_control_base_t *tc_, size_t n_) :
    n (n_), tc(tc_), head(0), used(0),
    throughput(0),
    a((::std::atomic<void*>*)calloc (n_ , sizeof (void*)))
  {}

  // the destructor is not safe!
  // to make it safe one needs to be sure the queue is empty
  // AND that no more values will be enqueued.
  // This is very hard to do. Using a smart ptr for the bag
  // ensures there will be no more enqueue operations started
  // but not that one is not in progress. The queue may appear
  // empty during the progress of such final enqueue operations.
  // there is no safe way to ensure the queue will remain empty.
  pthread_lf_bag::~pthread_lf_bag() { }

  void pthread_lf_bag::enqueue(void *d)
  {
wait:
    size_t stime = 1;
    while (used.load(::std::memory_order_seq_cst) == n) sleep(tc,stime);
    size_t i = (head + used) % n;
    while
    (
      (d = ::std::atomic_exchange_explicit(a + i, d,
        NQFENCE))
    )
    {
      if (used.load(::std::memory_order_seq_cst) == n) goto wait; // lost the race
      i = (i + 1) % n;
      if (i == head) sleep(tc,stime);
    }
    ++used;
  }

  void *pthread_lf_bag::dequeue ()
  {
wait:
    size_t stime = 1;
    while (used.load(::std::memory_order_seq_cst) == 0) sleep(tc,stime );

    size_t i = head.load(::std::memory_order_seq_cst);
    void *d = nullptr;
    while
    (
      !(d = ::std::atomic_exchange_explicit(a + i, d,
        DQFENCE))
    )
    {
      if (used.load(::std::memory_order_seq_cst) == 0) goto wait; // lost the race
      i = (i + 1) % n;
      if (i == head) sleep(tc,stime);
    }
    head.store (i,::std::memory_order_seq_cst);
    --used;
    ++throughput;
    return d;
  }
