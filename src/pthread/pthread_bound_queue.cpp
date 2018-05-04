#line 1492 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "pthread_bound_queue.hpp"
#include <queue>        // stl to the bloated rescue
#include <stdio.h>      // debugging in scanner

using namespace std;

namespace flx { namespace pthread {
typedef deque<void*> void_queue;

#define ELTQ ((void_queue*)lame_opaque)

void bound_queue_t::notify_world_stop()
{
  size_changed.notify_all();
}

bound_queue_t::bound_queue_t(thread_control_base_t *tc_, size_t n) : bound(n), tc(tc_)
{
//fprintf(stderr, "Creating bound queue %p, thread_control base=%p\n", this,tc);
  lame_opaque = new void_queue;
  tc->register_world_stop_notifier(this);
}

// Much care is needed deleting a queue.
// A safe method is possible .. but not provided here
bound_queue_t::~bound_queue_t()
{
//fprintf(stderr,"Deleting bound queue %p\n",this);
  tc->unregister_world_stop_notifier(this);
  delete ELTQ;
}

void bound_queue_t::wait() {
//fprintf(stderr, "Bound queue waiting.. %p\n", this);
  member_lock.unlock();
//fprintf(stderr, "Unocked mutex, now doing a tc yield q=%p, tc=%p\n", this,tc);
  tc->yield();
//fprintf(stderr, "tc yield done, relocking mutex q=%p\n", this);
  member_lock.lock();
//fprintf(stderr, "locked mutex again, waiting on possible size change in queue %p\n",this);
  size_changed.wait_for(member_lock, ::std::chrono::duration<int>(1)); // 1second
//fprintf(stderr, "possible size change in queue detected %p\n", this);
}

void bound_queue_t::wait_no_world_stop_check() {
  size_changed.wait_for(member_lock, ::std::chrono::duration<int>(1)); // 1second
}


// get the number of element in the queue
// (NOT the bound!)
size_t bound_queue_t::len() {
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  return ELTQ->size();
}

void bound_queue_t::wait_until_empty() {
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  while(!ELTQ->empty()) wait();
}

void
bound_queue_t::enqueue(void* elt)
{
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  while(ELTQ->size() >= bound) wait(); // guard against spurious wakeups!
  ELTQ->push_back(elt);
  size_changed.notify_all(); // cannot return an error
}

void
bound_queue_t::enqueue_no_world_stop_check(void* elt)
{
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  while(ELTQ->size() >= bound) wait_no_world_stop_check(); // guard against spurious wakeups!
  ELTQ->push_back(elt);
  size_changed.notify_all(); // cannot return an error
}


void*
bound_queue_t::dequeue()
{
//fprintf(stderr, "Trying to dequeue from bound queue\n");
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  while(ELTQ->empty())  wait(); // guard against spurious wakeups!
  void *elt = ELTQ->front();
  ELTQ->pop_front();
  size_changed.notify_all();
  return elt;
}

void*
bound_queue_t::maybe_dequeue()
{
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  void *elt = NULL;
  if (ELTQ->size() > 0)
  {
    elt = ELTQ->front();
    ELTQ->pop_front();
    size_changed.notify_all();
  }
  return elt;
}


void
bound_queue_t::resize(size_t n)
{
  ::std::unique_lock< ::std::mutex>   l(member_lock);
  bound = n;
  // get things rolling again
  size_changed.notify_all();
}

using namespace flx;;
using namespace gc;
using namespace generic;

void *bound_queue_scanner(
  collector_t *collector,
  gc_shape_t *shape, void *pp,
  size_t dyncount,
  int reclimit
)
{
  // input is a pointer to a pointer to a bound queue object
  void *p = *(void**)pp;
  bound_queue_t *bq = (bound_queue_t*)p;
  void_queue *pq = (void_queue*) bq->lame_opaque;
  printf("Scanning bound queue %p->%p\n", pp, p);

  ::std::deque<void*>::const_iterator stl_end = pq->end();
  for(
    ::std::deque<void*>::const_iterator iter= pq->begin();
    iter < stl_end;
    ++iter
  ) {
    void *value = *iter;
    printf("bound_queue scanning p=%p\n",value);
    collector->register_pointer(value,reclimit);
  }
  return 0;
}


}}


