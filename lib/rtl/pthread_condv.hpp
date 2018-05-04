#line 583 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __FLX_PTHREAD_CONDV_HPP__
#define __FLX_PTHREAD_CONDV_HPP__
#include <condition_variable>
#include <chrono>
#include "flx_pthread_config.hpp"
#include "pthread_thread_control_base.hpp"

namespace flx { namespace pthread {
class PTHREAD_EXTERN flx_condv_t : public world_stop_notifier_t
{
  ::std::mutex m;
  ::std::condition_variable_any cv;
  void notify_world_stop() override;
  thread_control_base_t *tc;
public:
   flx_condv_t (thread_control_base_t *);
   void lock();
   void unlock();
   void wait();
   void timed_wait(double seconds);
   void signal();
   void broadcast();
   ~flx_condv_t();
};

}}
#endif
