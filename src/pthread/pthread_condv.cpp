#line 612 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "pthread_condv.hpp"
#include <stdint.h>

namespace flx { namespace pthread {
// constructor
flx_condv_t::flx_condv_t(thread_control_base_t *tc_): tc(tc_) {
//fprintf(stderr, "Creating condition variable %p\n", this);
  tc->register_world_stop_notifier(this);
}

void flx_condv_t::notify_world_stop() { cv.notify_all(); }

void flx_condv_t::lock() { m.lock(); }

void flx_condv_t::unlock() { m.unlock(); }

// mutex must be LOCKED on entry to WAIT
// mutex will be LOCKED on exit from WAIT
void flx_condv_t::wait() {
  m.unlock();
  tc->yield();
  m.lock();
  cv.wait_for(m,::std::chrono::seconds (1));  // unlocks mutex on entry, relocks on exit
}

void flx_condv_t::timed_wait(double seconds) {
  m.unlock();
  tc->yield();
  m.lock();
  cv.wait_for(m,::std::chrono::microseconds ((uint64_t)(seconds*1000000.0)));
}

void flx_condv_t::signal() { cv.notify_one(); }

void flx_condv_t::broadcast() { cv.notify_all(); }

flx_condv_t::~flx_condv_t() { tc->unregister_world_stop_notifier(this); }

}}
