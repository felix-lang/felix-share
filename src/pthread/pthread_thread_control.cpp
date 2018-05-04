#line 958 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "pthread_thread.hpp"
#include <stdio.h>
#include <cstdlib>
#include <cassert>

#define FLX_SAVE_REGS \
  jmp_buf reg_save_on_stack; \
  setjmp (reg_save_on_stack)


namespace flx { namespace pthread {

world_stop_notifier_t::~world_stop_notifier_t(){}

static void *get_stack_pointer() {
  void *x;
  void *y = (void*)&x;
  return y;
}

// SHOULD BE MUTEX PROTECETD
void thread_control_t::register_world_stop_notifier(world_stop_notifier_t *p)
{
//fprintf(stderr,"World stop notifier registered: %p\n", p);
  for (size_t i=0; i<world_stop_notifier_array_length; ++i)
    if(p == world_stop_notifier_array[i]) return;
  world_stop_notifier_array = (world_stop_notifier_t**)realloc(world_stop_notifier_array,
    sizeof(world_stop_notifier_t*) * (world_stop_notifier_array_length + 1));
  world_stop_notifier_array[world_stop_notifier_array_length] = p;
  ++world_stop_notifier_array_length;
}

// SHOULD BE MUTEX PROTECETD
void thread_control_t::unregister_world_stop_notifier(world_stop_notifier_t *p)
{
  size_t i = 0;
  for (i=0; i<world_stop_notifier_array_length; ++i)
    if(p == world_stop_notifier_array[i]) break;
  if (i == world_stop_notifier_array_length) return;
  for (size_t j =  i + 1; j < world_stop_notifier_array_length; ++j)
    world_stop_notifier_array[j-1] = world_stop_notifier_array[j];
  --world_stop_notifier_array_length;
  world_stop_notifier_array = (world_stop_notifier_t**)realloc(world_stop_notifier_array,
    sizeof(world_stop_notifier_t*) * (world_stop_notifier_array_length));
}

void thread_control_t::world_stop_notify()
{
if (world_stop_notifier_array_length > 0)
  //fprintf(stderr, "thread_control_t::world_stop_notify() notifying %zu objects\n",
  //  world_stop_notifier_array_length);
  for (size_t i=0; i<world_stop_notifier_array_length; ++i)
    world_stop_notifier_array[i]->notify_world_stop();
}

bool thread_control_t::get_debug()const { return debug; }

thread_control_base_t::~thread_control_base_t(){}

thread_control_t::thread_control_t (bool d) :
  do_world_stop(false), thread_counter(0), active_counter(0), debug(d),
  world_stop_notifier_array(0), world_stop_notifier_array_length(0)
  {
    if(debug)
      fprintf(stderr,"INITIALISING THREAD CONTROL OBJECT\n");
  }

size_t thread_control_t::thread_count()
  {
    ::std::unique_lock< ::std::mutex> m(stop_mutex);
    return thread_counter;
  }

size_t thread_control_t::active_count()
  {
    ::std::unique_lock< ::std::mutex> m(stop_mutex);
    return active_counter;
  }

void thread_control_t::add_thread(void *stack_base)
  {
    ::std::unique_lock< ::std::mutex> m(stop_mutex);
    uintptr_t id = mythrid();
    threads.insert (std::make_pair(id, thread_data_t (stack_base)));
    ++thread_counter;
    ++active_counter;
    if(debug)
      fprintf(stderr, "Adding thread %p base %p, count=%zu\n", (void*)(uintptr_t)id, stack_base, thread_counter);
    stop_guard.notify_all();
  }

void thread_control_t::remove_thread()
  {
    ::std::unique_lock< ::std::mutex> m(stop_mutex);
    uintptr_t id = mythrid();
    if (threads.erase(id) == 0)
    {
      fprintf(stderr, "Remove thread %p which is not registered\n", (void*)(uintptr_t)id);
      std::abort();
    }
    --thread_counter;
    --active_counter;
    if(debug)
      fprintf(stderr, "Removed thread %p, count=%zu\n", (void*)(uintptr_t)id, thread_counter);
    stop_guard.notify_all();
  }

// stop the world!

// NOTE: ON EXIT, THE MUTEX REMAINS LOCKED

bool thread_control_t::world_stop()
  {
    stop_mutex.lock();
    if(debug)
      fprintf(stderr,"Thread %p Stopping world, active threads=%zu\n", (void*)mythrid(), active_counter);
    if (do_world_stop) {
      stop_mutex.unlock();
      return false; // race! Someone else beat us
    }
    do_world_stop = true;

    // this calls the notify_world_stop() method of all the
    // objects such as condition variables that are registered
    // in the notification list. That method is expected to do a notify_all()
    // on the condition variable.

    world_stop_notify();

    // this is for the thread control objects own condition variable
    // which is used to count the number of threads that have suspended

    stop_guard.notify_all();

    while(active_counter>1) {
      if(debug)
        for(
          thread_registry_t::iterator it = threads.begin();
          it != threads.end();
          ++it
        )
        {
          fprintf(stderr, "Thread = %p is %s\n",(void*)(uintptr_t)(*it).first, (*it).second.active? "ACTIVE": "SUSPENDED");
        }
      if(debug)
        fprintf(stderr,"Thread %p Stopping world: begin wait, threads=%zu\n",  (void*)mythrid(), thread_counter);
      stop_guard.wait(stop_mutex);
      if(debug)
        fprintf(stderr,"Thread %p Stopping world: checking threads=%zu\n", (void*)mythrid(), thread_counter);
    }
    // this code has to be copied here, we cannot use 'yield' because
    // it would deadlock ourself
    {
      uintptr_t id = mythrid();
      FLX_SAVE_REGS;
      void *stack_pointer = get_stack_pointer();
      if(debug)
        fprintf(stderr,"World stop thread=%p, stack=%p!\n",(void*)(uintptr_t)id, stack_pointer);
      thread_registry_t::iterator it = threads.find(id);
      if(it == threads.end()) {
        fprintf(stderr,"MAIN THREAD: Cannot find thread %p in registry\n",(void*)(uintptr_t)id);
        abort();
      }
      (*it).second.stack_top = stack_pointer;
      if(debug)
        fprintf(stderr,"Stack size = %zu\n",(size_t)((char*)(*it).second.stack_base -(char*)(*it).second.stack_top));
    }
    if(debug)
      fprintf(stderr,"World STOPPED\n");
    return true; // we stopped the world
  }

// used by mainline to wait for other threads to die
void thread_control_t::join_all()
  {
    ::std::unique_lock< ::std::mutex> m(stop_mutex);
    if(debug)
      fprintf(stderr,"Thread %p Joining all\n", (void*)mythrid());
    while(do_world_stop || thread_counter>1) {
      unsafe_stop_check();
      stop_guard.wait(stop_mutex);
    }
    if(debug)
      fprintf(stderr,"World restarted: do_world_stop=%d, Yield thread count now %zu\n",do_world_stop,thread_counter);
  }

// restart the world
void thread_control_t::world_start()
  {
    if(debug)
      fprintf(stderr,"Thread %p Restarting world\n", (void*)mythrid());
    do_world_stop = false;
    stop_mutex.unlock();
    stop_guard.notify_all();
  }

memory_ranges_t *thread_control_t::get_block_list()
{
  memory_ranges_t *v = new std::vector<memory_range_t>;
  thread_registry_t::iterator end = threads.end();
  for(thread_registry_t::iterator i = threads.begin();
    i != end;
    ++i
  )
  {
    thread_data_t const &td = (*i).second;
    // !(base < top) means top <= base, i.e. stack grows downwards
    assert(!std::less<void*>()(td.stack_base,td.stack_top));
    // from top upto base..
    v->push_back(memory_range_t(td.stack_top, td.stack_base));
  }
  return v;
}

void thread_control_t::suspend()
{
  ::std::unique_lock< ::std::mutex> m(stop_mutex);
  if(debug)
    fprintf(stderr,"[suspend: thread= %p]\n", (void*)mythrid());
  unsafe_suspend();
}

void thread_control_t::resume()
{
  ::std::unique_lock< ::std::mutex> m(stop_mutex);
  if(debug)
    fprintf(stderr,"[resume: thread= %p]\n", (void*)mythrid());
  unsafe_resume();
}


void thread_control_t::unsafe_suspend()
{
  void *stack_pointer = get_stack_pointer();
  uintptr_t id = mythrid();
  if(debug)
    fprintf(stderr,"[unsafe_suspend:thread=%p], stack=%p!\n",(void*)(uintptr_t)id, stack_pointer);
  thread_registry_t::iterator it = threads.find(id);
  if(it == threads.end()) {
    if(debug)
      fprintf(stderr,"[unsafe_suspend] Cannot find thread %p in registry\n",(void*)(uintptr_t)id);
      abort();
  }
  (*it).second.stack_top = stack_pointer;
  (*it).second.active = false;
  if(debug) // VC++ is bugged, doesn't support %td format correctly?
    fprintf(stderr,"[unsafe_suspend: thread=%p] stack base %p > stack top %p, Stack size = %zd\n",
      (void*)(uintptr_t)id,
      (char*)(*it).second.stack_base,
      (char*)(*it).second.stack_top,
      (size_t)((char*)(*it).second.stack_base -(char*)(*it).second.stack_top));
  --active_counter;
  if(debug)
    fprintf(stderr,"[unsafe_suspend]: active thread count now %zu\n",active_counter);
  stop_guard.notify_all();
  if(debug)
    fprintf(stderr,"[unsafe_suspend]: stop_guard.notify_all() done");
}

void thread_control_t::unsafe_resume()
{
  if(debug)
    fprintf(stderr,"[unsafe_resume: thread %p]\n", (void*)mythrid());
  stop_guard.notify_all();
  if(debug)
    fprintf(stderr,"[unsafe_resume]: stop_guard.notify_all() done");
  while(do_world_stop) stop_guard.wait(stop_mutex);
  if(debug)
    fprintf(stderr,"[unsafe_resume]: stop_guard.wait() done");
  ++active_counter;
  uintptr_t id = mythrid();
  thread_registry_t::iterator it = threads.find(id);
  if(it == threads.end()) {
    if(debug)
      fprintf(stderr,"[unsafe_resume: thread=%p] Cannot find thread in registry\n",(void*)(uintptr_t)id);
      abort();
  }
  (*it).second.active = true;
  if(debug) {
    fprintf(stderr,"[unsafe_resume: thread=%p] resumed, active count= %zu\n",
      (void*)mythrid(),active_counter);
  }
  stop_guard.notify_all();
  if(debug)
    fprintf(stderr,"[unsafe_resume]: stop_guard.notify_all() done");
}

// mutex already held
void thread_control_t::unsafe_stop_check()
{
//fprintf(stderr, "Unsafe stop check ..\n");
  if (do_world_stop)
  {

    if(debug)
      fprintf(stderr,"[unsafe_stop_check: thread=%p] world_stop detected\n",
        (void*)mythrid());
    FLX_SAVE_REGS;
    unsafe_suspend();
    unsafe_resume();
  }
//fprintf(stderr, "Unsafe stop check finishes\n");
}

void thread_control_t::yield()
{
//fprintf(stderr,"Thread control yield starts\n");
  ::std::unique_lock< ::std::mutex> m(stop_mutex);
  if(debug)
    fprintf(stderr,"[yield: thread=%p]\n", (void*)mythrid());
//fprintf(stderr,"Unsafe stop check starts\n");
  unsafe_stop_check();
//fprintf(stderr,"Unsafe stop check done\n");
}

}}
