#line 1781 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __pthread_fast_lock__
#define __pthread_fast_lock__
#include "flx_pthread_config.hpp"
#include "pthread_thread_control_base.hpp"
#include <atomic>

namespace flx { namespace rtl {

class PTHREAD_EXTERN fast_lock
{
  ::std::atomic_flag flag;
  ::flx::pthread::thread_control_base_t *tc;
public:
  fast_lock(::flx::pthread::thread_control_base_t *);
  fast_lock() = delete;
  fast_lock(fast_lock const&)  = delete;
  void operator = (fast_lock const&) = delete;
  void lock();
  void unlock();
};
}}
#endif
