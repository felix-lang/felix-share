#line 384 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "pthread_thread.hpp"
#if FLX_WIN32
#include <stdio.h>
#include <cstdlib>
#include <assert.h>

namespace flx { namespace pthread {

flx_native_thread_t get_current_native_thread() { return GetCurrentThread(); }
uintptr_t mythrid() { return (uintptr_t)GetCurrentThreadId(); }

static void *get_stack_pointer() {
  void *x;
  void *y = (void*)&x;
  return y;
}

DWORD WINAPI flx_pthread_start_wrapper(LPVOID e)
{
  void *stack_base = get_stack_pointer();
  tstart_t *ehd = (tstart_t*)e;
  thread_control_base_t *tc = ehd -> tc;
  if(tc == 0)
  {
    fprintf(stderr, "ERROR: flx_pthread_start_wrapper got NULL thread control object\n");
    assert(tc);
  }
  bool debug = tc->get_debug();
  if(debug)
    fprintf(stderr,"Spawned Thread %p start stack base = %p, tc=%p\n",
       (void*)mythrid(),stack_base, tc);
  if(debug)
    fprintf(stderr,"Spawned Thread %p start stack base = %p, tc=%p\n",(void*)mythrid(),stack_base, tc);
  if(tc->get_debug())
    fprintf(stderr,"Thread registering itself\n");
  tc->add_thread(stack_base);
  if(debug)
    fprintf(stderr,"Registered: Spawned Thread %p stack base = %p\n",
      (void*)mythrid(),stack_base, tc);
  void (*sr)(void*)=ehd->sr;
  void *cd = ehd->cd;
  if(debug)
    fprintf(stderr,"ehd->spawner_lock = %p\n",ehd->spawner_lock);

  if(ehd->spawner_lock)
  {
    ::std::unique_lock< ::std::mutex> dummy(*ehd->spawner_lock);
    if (debug)
      fprintf(stderr,"Thread %p acquired mutex\n", (void*)mythrid());
    if (debug)
      fprintf(stderr,"Thread %p notifying spawner it has registered itself\n", (void*)mythrid());
    *ehd->spawner_flag=true;
    ehd->spawner_cond->notify_all();
    if (debug)
      fprintf(stderr,"Thread %p releasing mutex\n", (void*)mythrid());
  }
  delete ehd;
  if (debug)
    fprintf(stderr,"Thread %p yielding\n", (void*)mythrid());
  tc->yield();
  try {
    if (debug)
      fprintf(stderr,"Thread %p running client code\n", (void*)mythrid());
    (*sr)(cd);
  }
  catch (...) {
    fprintf(stderr,"Uncaught exception in thread\n");
    ::std::exit(1);
  }
  if (debug)
    fprintf(stderr,"Thread %p unregistering\n", (void*)mythrid());
  tc->remove_thread();
  return 0;
}

DWORD WINAPI nonflx_pthread_start_wrapper(LPVOID e)
{
  void *stack_base = get_stack_pointer();
  tstart_t *ehd = (tstart_t*)e;
  void (*sr)(void*)=ehd->sr;
  void *cd = ehd->cd;
  if(ehd->spawner_lock)
  {
    ::std::unique_lock< ::std::mutex> dummy(*ehd->spawner_lock);
    *ehd->spawner_flag=true;
    ehd->spawner_cond->notify_all();
  }
  delete ehd;
  try {
    (*sr)(cd);
  }
  catch (...) {
    fprintf(stderr,"Uncaught exception in thread\n");
    ::std::exit(1);
  }
  return 0;
}


// ---- detached threads ----------

flx_detached_thread_t::flx_detached_thread_t(flx_detached_thread_t const&){} // uncopyable
void flx_detached_thread_t::operator=(flx_detached_thread_t const&){} // uncopyable

// returns -1 on failure with error in GetLastError, 0 if all good.
int
flx_detached_thread_t::init(void (*start)(void*), void *lParam, thread_control_base_t *tc,
  ::std::mutex * m, ::std::condition_variable_any *c,bool *flag)
{
  DWORD thread_id = 0;
  thr = (HANDLE)CreateThread(NULL, 0,
    (LPTHREAD_START_ROUTINE)flx_pthread_start_wrapper,
    new tstart_t(start,lParam, tc, m, c, flag), 0,
    &thread_id
  );

  if(!thr)
  {
    DWORD err = GetLastError();
    fprintf(stderr, "flx_detached_thread_t: CreateThread failed: %i\n", err);
    return err;
  }
  return 0;
}

flx_detached_thread_t::~flx_detached_thread_t() { CloseHandle(thr); }
flx_detached_thread_t::flx_detached_thread_t() { }

// ---- joinable threads ----------
flx_thread_t::flx_thread_t(flx_thread_t const&){} // uncopyable
void flx_thread_t::operator=(flx_thread_t const&){} // uncopyable


flx_thread_t::flx_thread_t() { }
flx_thread_t::~flx_thread_t() { }

// this should be idempotent
void
flx_thread_t::join()
{
  // Let's try and wait for the thread to finish, however first I have to
  // tell it to finish up.

  DWORD  wait_res = WaitForSingleObject(thr, INFINITE);

  // will this give me my return status? how do I get that?
  if(WAIT_FAILED == wait_res)
  {
    fprintf(stderr,"WARNING: thread wait failed (%li)\n", GetLastError());
  }

  // I've already tried waiting on the  thread's #include <stdlib> exit
  if(!CloseHandle(thr))
  {
    fprintf(stderr,"FATAL: failed to delete thread (%li)\n", GetLastError());
    std::exit(1);
  }
}

// returns -1 on failure with error in GetLastError, 0 if all good.
int
flx_thread_t::init(void (*fn)(void*), void *lParam, thread_control_base_t *tc)
{
  DWORD thread_id = 0;
  thr= (HANDLE)CreateThread(NULL, 0,
    (LPTHREAD_START_ROUTINE)nonflx_pthread_start_wrapper,
    new tstart_t(fn,lParam, tc,NULL,NULL,NULL), 0,
    &thread_id
  );

  if(!thr)
  {
    DWORD err = GetLastError();
    fprintf(stderr, "WARNING: flx_thread_t: CreateThread failed: %i\n", err);
    return err;
  }

  return 0;
}

// ---- joinable thread wrapper ----------
flx_thread_wrapper_t::flx_thread_wrapper_t(void (*f)(void*), void *lParam, thread_control_base_t*tc)
{
  int res = thread.init(f,lParam,tc);
  if(res)
  {
    fprintf(stderr,"flx_thread_wrapper_t: FATAL: flx_thread_t.init failed\n");
    std::exit(1);
  }
}
flx_thread_wrapper_t::~flx_thread_wrapper_t() { thread.join(); }

}}

#endif
