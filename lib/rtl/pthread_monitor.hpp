#line 655 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __FLX_PTHREAD_MONITOR_H__
#define __FLX_PTHREAD_MONITOR_H__
#include "flx_pthread_config.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include "pthread_thread_control_base.hpp"

// interface for a consumer/producer queue. threads requesting a resource
// that isn't there block until one is available. push/pop re-entrant

namespace flx { namespace pthread {

struct monitor_data_t
{
  void *user_data;
  ::std::atomic<bool> flag;
  monitor_data_t (void* u) : user_data(u), flag(false) {}
};


class PTHREAD_EXTERN monitor_t {
  ::std::atomic<monitor_data_t*> volatile data;
  thread_control_base_t *tc;
public:
  monitor_t(thread_control_base_t *);
  ~monitor_t();
  void enqueue(void*);
  void* dequeue();
};

}} // namespace pthread, flx
#endif

