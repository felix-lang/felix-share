#line 190 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "pthread_thread.hpp"
#if FLX_POSIX
#include <stdio.h>
#include <string.h>  // strerror
#include <cstdlib>
#include <setjmp.h>
#include <functional> // less
#include <assert.h>

namespace flx { namespace pthread {

flx_native_thread_t get_current_native_thread() { return pthread_self(); }
uintptr_t mythrid() { return (uintptr_t)pthread_self(); }

static void *get_stack_pointer() {
  void *x;
  void *y = (void*)&x;
  return y;
}

extern "C" void *flx_pthread_start_wrapper(void *e)
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
      fprintf(stderr,"Thread registering itself\n");
  tc->add_thread(stack_base);
  if(debug)
    fprintf(stderr,"Registered: Spawned Thread %p stack base = %p\n",
      (void*)mythrid(),stack_base, tc);


  void (*sr)(void*)=ehd->sr; // client function
  void *cd = ehd->cd;        // client data
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
  return NULL;
}


extern "C" void *nonflx_pthread_start_wrapper(void *e)
{
  void *stack_base = get_stack_pointer();
  tstart_t *ehd = (tstart_t*)e;
  void (*sr)(void*)=ehd->sr; // client function
  void *cd = ehd->cd;        // client data

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
  return NULL;
}


// ---- detached threads ----------

flx_detached_thread_t::flx_detached_thread_t(flx_detached_thread_t const&){} // uncopyable
void flx_detached_thread_t::operator=(flx_detached_thread_t const&){} // uncopyable

int
flx_detached_thread_t::init(void (*start)(void*), void* udat, thread_control_base_t *tc,
  ::std::mutex * m, ::std::condition_variable_any *c,bool *flag)
{
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  int res = pthread_create(&thr, &attr, flx_pthread_start_wrapper,
    new tstart_t(start, udat, tc, m,c,flag));
  if(res)
  {
     fprintf(stderr, "WARNING: flx_detached_thread_t: pthread_create failed: %s\n",
       strerror(res));
  }
  pthread_attr_destroy(&attr);
  return res;
}

flx_detached_thread_t::~flx_detached_thread_t() { }
flx_detached_thread_t::flx_detached_thread_t() { }

// ---- joinable threads ----------
flx_thread_t::flx_thread_t(flx_thread_t const&){} // uncopyable
void flx_thread_t::operator=(flx_thread_t const&){} // uncopyable

int
flx_thread_t::init(void (*start)(void*), void* udat, thread_control_base_t*tc)
{
  int res = pthread_create(&thr, NULL, nonflx_pthread_start_wrapper,
    new tstart_t(start, udat, tc,NULL,NULL,NULL));
  if(res)
  {
     fprintf(stderr, "WARNING: flx_thread_t: pthread_create failed: %s\n",
       strerror(res));
  }
  return res;
}

void flx_thread_t::join() {
  int res = pthread_join(thr, NULL);
  if(res)
  {
     fprintf(stderr, "flx_thread_t: FATAL: pthread_join failed: %s\n",
       strerror(res));
#ifdef exit
     // Someone wants to replace exit with their own thing ...
     exit(1);
#else
     std::exit(1);
#endif
  }
}

flx_thread_t::~flx_thread_t() { }
flx_thread_t::flx_thread_t() { }

// ---- joinable thread wrapper ----------

flx_thread_wrapper_t::flx_thread_wrapper_t(flx_thread_wrapper_t const&){} // uncopyable
void flx_thread_wrapper_t::operator=(flx_thread_wrapper_t const&){} // uncopyable

flx_thread_wrapper_t::flx_thread_wrapper_t(void (*start)(void*), void* udat, thread_control_base_t*tc)
{
  int res = thread.init(start,udat,tc);
  {
    if(res)
    {
       fprintf(stderr, "FATAL: flx_thread_wapper_t: flx_thread_t.init failed: %s\n",
         strerror(res));
#ifdef exit
     // Someone wants to replace exit with their own thing ...
     exit(1);
#else
     std::exit(1);
#endif
    }
  }
}

flx_thread_wrapper_t::~flx_thread_wrapper_t() { thread.join(); }
}}

#endif
