#line 36 "/Users/skaller/felix/src/packages/sync.fdoc"

#ifndef __FLX_SYNC_H__
#define __FLX_SYNC_H__

#include "flx_gc.hpp"
#include "flx_rtl.hpp"
#include <list>

namespace flx { namespace run {

// This class handles synchronous channel I/O and fthreads
struct RTL_EXTERN sync_sched {
  bool debug_driver;

  // the garbage collector and general control object
  ::flx::gc::generic::collector_t *collector;

  // scheduler queue
  ::std::list<flx::rtl::fthread_t*> *active;
  ::std::list<flx::rtl::fthread_t*> *waiting;

  // temporary for currently running fibre
  ::flx::rtl::fthread_t *ft;

  // variable to hold service request
  ::flx::rtl::_uctor_ *request;

  // type for the state of the scheduler
  // when it suspends by returning.
  enum fstate_t { blocked, delegated };

  // debugging helper to get a description of
  // the suspended scheduler state
  static char const * get_fstate_desc(fstate_t);

  // debugging helper to get a description of
  // the running scheduler state
  char const * get_fpc_desc();

  sync_sched (
    bool debug_driver_,
    ::flx::gc::generic::gc_profile_t *gcp_,
    ::std::list<flx::rtl::fthread_t*> *active_
  );

  // helper routines.
  void forget_current();
  void pop_current();
  void push_new(::flx::rtl::fthread_t*);
  void push_old(::flx::rtl::fthread_t*);

  fstate_t frun();

  // a special routine to allow a multiwrite to be performed
  // from outside the scheduler whilst it is suspended.
  void external_multi_swrite(::flx::rtl::schannel_t*, void*);
protected:
  // handlers for synchronous service calls.
  void do_yield();
  void do_swait();
  void do_spawn_detached();
  void do_schedule_detached();
  void do_sread();
  void do_swrite();
  void do_multi_swrite();
  void do_kill();
  void show_state();
};

}}

#endif
