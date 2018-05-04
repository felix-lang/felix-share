#line 1005 "/Users/skaller/felix/src/packages/gc.fdoc"

#include <cstdlib>
#include <map>
#include <limits.h>
#include <cassert>
#include <cstdio>
#include <cstddef>
#include "flx_rtl_config.hpp"
#include "flx_collector.hpp"
#include "flx_exceptions.hpp"
#include "flx_gc_private.hpp"

#include <stdint.h>
#define lobit(p) (p & (uintptr_t)1u)
#define hibits(p) (p & ~(uintptr_t)1u)
#define SHAPE(p) ((gc_shape_t *)hibits(p))

//#include "flx_rtl.hpp"
namespace flx {
namespace gc {
namespace collector {

static int mcount FLX_UNUSED = 0;

malloc_free::~malloc_free(){}

void *malloc_free::allocate(::std::size_t amt)
{
  void *p = malloc(amt);
  if(debug)
    fprintf(stderr,"[gc] Malloc %zd bytes, address = %p\n",amt,p);
  if(p)return p;
  else {
    fprintf(stderr,"[gc] Felix: Malloc out of memory, blk=%zu\n",amt);
    throw flx::rtl::flx_out_of_memory_t();
  }
}

void malloc_free::deallocate(void *p)
{
  if(debug)
    fprintf(stderr,"[gc] Free %p\n",p);
  free(p);
}

tracing_allocator::tracing_allocator (
  FILE *tf,
  allocator_t *slave)
: tracefile(tf), servant(slave) {}

void *tracing_allocator::allocate (::std::size_t amt)
{
   void *memory = servant->allocate(amt);
   fprintf(tracefile,"A: %p\n",memory);
   return memory;
}

void tracing_allocator::deallocate (void *p)
{
   fprintf(tracefile,"D: %p\n",p);
   servant->deallocate(p);
}

tracing_allocator::~tracing_allocator() {
  fclose(tracefile);
  delete servant;
  fprintf(stderr, "[gc] Allocation tracing terminated, file closed, slave allocator deleted\n");
}


void *flx_collector_t::v_allocate(gc_shape_t *ptr_map, size_t x) {
  return impl_allocate(ptr_map, x);
}

void flx_collector_t::v_finalise(void *frame) {
  impl_finalise(frame);
}

size_t flx_collector_t::v_collect() {
  // NO MUTEX
  return impl_collect();
}

void flx_collector_t::v_add_root(void *memory) {
  impl_add_root(memory);
}

void flx_collector_t::v_remove_root(void *memory) {
  impl_remove_root(memory);
}

void flx_collector_t::v_free_all_mem() {
  //fprintf(stderr, "Dispatching to impl free all mem\n");
  impl_free_all_mem();
}

size_t flx_collector_t::v_get_allocation_count()const {
  return impl_get_allocation_count();
}

size_t flx_collector_t::v_get_root_count()const {
  return impl_get_root_count();
}

size_t flx_collector_t::v_get_allocation_amt()const {
  return impl_get_allocation_amt();
}

size_t flx_collector_t::impl_get_allocation_count()const
{
  return allocation_count;
}

size_t flx_collector_t::impl_get_root_count()const
{
  return root_count;
}

size_t flx_collector_t::impl_get_allocation_amt()const
{
  return allocation_amt;
}


flx_collector_t::flx_collector_t(
  allocator_t *a,
  pthread::thread_control_base_t *tc,
  int _gcthreads,
  FILE *tf
)
  :
  allocation_count(0)
  ,root_count(0)
  ,allocation_amt(0)
  ,parity(false)
  ,allocator(a)
  ,thread_control(tc)
  ,j_shape(0)
  ,j_nalloc(0)
  ,j_nused(0)
  ,j_tmp(0)
  ,minptr(~uintptr_t(0))
  ,maxptr(0)
  ,tracefile(tf)
  ,gcthreads(_gcthreads)
{
  if(tf)
    fprintf(stderr, "[flx_collector_t] Tracefile active\n");
}

flx::pthread::thread_control_base_t *flx_collector_t::get_thread_control()const
{
  return thread_control;
}

void flx_collector_t::judyerror(char const *loc)
{
  fprintf(stderr, "[gc] JUDY ERROR %d in %s\n",je.je_Errno,loc);
  abort();
}

void * flx_collector_t::impl_allocate(gc_shape_t *shape, size_t nobj)
{
  // calculate how much memory to request
  ::std::size_t amt = nobj * shape->amt * shape->count;
  //fprintf(stderr, "req amt = %zu\n",amt);
  if(amt & 1) ++amt; // round up to even number
  //fprintf(stderr, "rounded req amt = %zu\n",amt);

  // allocate a block
  void *fp = (void *)allocator->allocate(amt);
  assert(fp); // Got some memory!

  //++shape->allocations;

  // for use when things go wrong
  char error_buffer[2048];
  snprintf(error_buffer, 2047,
    "[gc] Allocated %p, shape=%s[%zd][%zu][#a=%zu,#d=%zu]\n",
    fp,shape->cname,shape->count,nobj,shape->allocations,shape->deallocations);

  Word_t *p = (Word_t*)(void*)JudyLIns(&j_shape,(Word_t)fp,&je);
  if(tracefile)
     fprintf(tracefile,"IS: %p\n",fp);
  *p = ((Word_t)(void*)shape) | (parity & 1);
  if (nobj != (uintptr_t)1) // array
  {
//fprintf(stderr, "Inserting into j_nalloc=%p\n",j_nalloc);
    Word_t *p = (Word_t*)(void*)JudyLIns(&j_nalloc,(Word_t)fp,&je);
//fprintf(stderr, "  new j_nalloc=%p\n",j_nalloc);
//fprintf(stderr, "  slot for insert=%p\n",p);
    if(tracefile)
       fprintf(tracefile,"IA: %p\n",fp);
    *p = nobj;
  }

  size_t n_objects = get_count(fp);
  if (nobj != n_objects)
  {

    fprintf(stderr,
        "Insertion into j_nalloc (%p) failed: address %p, [nobj=%zu != get_count(fp)=%zu]\n",
        j_nalloc, fp, nobj, n_objects);

    { // get_count(fp) conflates size 1 with NULL pointer, the following will disambiguate
      Word_t *p = (Word_t*)(void*)JudyLGet(j_nalloc,(Word_t)fp,&je);
      fprintf(stderr,
          "  p==NULL: %s\n",
          ((p == NULL) ? "true" : "false") );
    }

    // finally output error_buffer if there's an error
    fprintf(stderr, "%s", error_buffer);

    assert (nobj == n_objects);
  }

  // update statistics
  allocation_count++;
  allocation_amt += amt;
  //fprintf(stderr,"ADDING %zu to allocation amt, result %zu\n",amt,allocation_amt);
  // return client memory pointer
  minptr=::std::min(minptr,uintptr_t(fp));
  maxptr=::std::max(maxptr,uintptr_t(fp)+amt);
  return fp;
}

// NOTE: although 1 is the default if there is no entry,
// it is allowed to have an entry with 1
// indeed, set_used always creates an entry
void flx_collector_t::set_used(void *memory, size_t n)
{
  if (memory == NULL && n==0) return;
  assert(memory);

  // this check is expensive, but set_used is not used often
  assert(n<=get_count(memory));
  //fprintf(stderr,"Set used of %p to %zu\n",memory,n);
  Word_t *p = (Word_t*)(void*)JudyLGet(j_nused,(Word_t)memory,&je);
  if(tracefile)
    fprintf(tracefile,"GU: %p\n",memory);
  if(p==(Word_t*)PPJERR)judyerror("set_used");
  if(p==NULL)
  {
    //fprintf(stderr,"set_used: No recorded usage! Creating store for data\n");
    p = (Word_t*)(void*)JudyLIns(&j_nused,(Word_t)memory,&je);
    if(tracefile)
       fprintf(tracefile,"IU: %p\n",memory);
  }
  //fprintf(stderr,"Slot for %p usage is address %p\n",memory,p);
  *p = (Word_t)n;
}

void flx_collector_t::incr_used(void *memory, ptrdiff_t n)
{
  if (n==0) return;
  assert(memory);
  //fprintf(stderr,"Incr used of %p by %zu\n",memory,n);
  //assert(get_used(memory) + n <= get_count(memory));
  ptrdiff_t newused = (ptrdiff_t)get_used(memory) + n;
  if (newused < 0 || newused > get_count(memory)) {
    fprintf(stderr,"Address %p count %d used %d increment %d\n",
      memory,(int)get_count(memory), (int)get_used(memory),(int)n);
    fprintf(stderr,"Type %s\n",get_shape(memory)->cname);
    assert(false);
  }

  Word_t *p = (Word_t*)(void*)JudyLGet(j_nused,(Word_t)memory,&je);
  if(tracefile)
    fprintf(tracefile,"GU: %p\n",memory);
  if(p==(Word_t*)PPJERR)judyerror("incr_used");
  if(p==NULL)
  {
    //fprintf(stderr,"incr_used: No recorded usage! Creating store for data\n");
    p = (Word_t*)(void*)JudyLIns(&j_nused,(Word_t)memory,&je);
    if(tracefile)
      fprintf(tracefile,"IU: %p\n",memory);
    if(p==(Word_t*)PPJERR)judyerror("incr_used: new slot");
    *p = newused;
  }
  else *p=newused;
}

// actual number of used slots in an array
size_t flx_collector_t::get_used(void *memory)
{
  if(memory==NULL) return 0;
  //fprintf(stderr, "Get used of %p\n",memory);
  Word_t *p = (Word_t*)(void*)JudyLGet(j_nused,(Word_t)memory,&je);
  if(tracefile)
    fprintf(tracefile,"GU: %p\n",memory);
  if(p==(Word_t*)PPJERR)judyerror("get_used");
  //fprintf(stderr, "Used slot at address %p\n",p);
  size_t z = p!=NULL?*p:1; // defaults to 1 for non-array support
  //fprintf(stderr,"Used of %p is %zu\n",memory,z);
  return z;
}

// max number of available slots in an array
size_t flx_collector_t::get_count(void *memory)
{
  if(memory==NULL) return 0;
  //fprintf(stderr, "Get count of %p\n",memory);
  Word_t *p = (Word_t*)(void*)JudyLGet(j_nalloc,(Word_t)memory,&je);
  if(tracefile)
    fprintf(tracefile,"GA: %p\n",memory);
  if(p==(Word_t*)PPJERR)judyerror("get_count");
  //fprintf(stderr, "Count slot at address %p\n",p);
  size_t z = p!=NULL?*p:1; // defaults to 1 for non-array support
  //fprintf(stderr,"Count of %p is %zu\n\n",memory,z);
  return z;
}

// REQUIRES memory to be head pointer (not interior)
gc_shape_t *flx_collector_t::get_shape(void *memory)
{
  if(memory == NULL) return &::flx::gc::generic::_ptr_void_map;
  assert(memory);
  //fprintf(stderr, "Get shape of %p\n",memory);
  Word_t *pshape= (Word_t*)JudyLGet(j_shape,(Word_t)memory,&je);
  if(tracefile)
    fprintf(tracefile,"GS: %p\n",memory);
  if(pshape==(Word_t*)PPJERR)judyerror("get_shape");
  if(pshape==NULL) {
    fprintf(stderr,"get_shape %p found NULL\n",memory);
    abort();
  }
  return (gc_shape_t *)(*pshape & (~(uintptr_t)1));
}

void *flx_collector_t::create_empty_array(
  flx::gc::generic::gc_shape_t *shape,
  size_t count
)
{
  if (count==0) return NULL;
  void *p = allocate(shape,count);
  assert(p);
  set_used (p, 0); // make sure to override default 1 slot usage
  if(get_used(p) != 0 || get_count(p) != count) {
    fprintf(stderr,"create empty array type %s address %p request count=%zu, actual count=%zu ,used=%zu\n",
     p,shape->cname, count, get_count(p), get_used(p));
    fprintf(stderr, "FATAL CONSTRUCTOR FAILURE\n");
    assert (false);
  }
  return p;
}


void flx_collector_t::impl_finalise(void *fp)
{
  assert(fp!=NULL);
  //fprintf(stderr, "Finaliser for %p\n", fp);
  gc_shape_t *shape = get_shape(fp); // inefficient, since we already know the shape!
  //fprintf(stderr, "Got shape %p=%s\n", shape,shape->cname);
  void (*finaliser)(collector_t*, void*) = shape->finaliser;
  //fprintf(stderr, "Got finaliser %p\n", finaliser);
  if (finaliser)
  {
    unsigned char *cp = (unsigned char*)fp;
    size_t n_used = get_used(fp) * shape->count;
    size_t eltsize = shape->amt;
    //fprintf(stderr, "Finalising at %p for type %s %zu objects each size %zu\n", cp, shape->cname, n_used, eltsize);
    for(size_t j = 0; j<n_used; ++j)
    {
      (*finaliser)(this,(void*)cp);
      cp += eltsize;
    }
  }
}

void flx_collector_t::unlink(void *fp)
{
  // check we have a pointer to an object
  assert(fp!=NULL);

  // call the finaliser if there is one
  //fprintf(stderr,"Unlink: Calling finaliser for %p\n",fp);
  impl_finalise(fp);

  allocation_count--;
  gc_shape_t *shape = get_shape(fp);
  size_t n_objects = get_count(fp);
  size_t nobj = shape -> count * n_objects;
  ::std::size_t size = shape->amt * nobj;
  if (size & 1) ++size;
  //fprintf(stderr, "Uncounting %zu bytes\n", size);
  allocation_amt -= size;

  // unlink the frame from the collectors list
  //fprintf(stderr,"Removing address from Judy lists\n");
  JudyLDel(&j_shape, (Word_t)fp, &je);
  JudyLDel(&j_nused, (Word_t)fp, &je);
  JudyLDel(&j_nalloc, (Word_t)fp, &je);
  if(tracefile) {
    fprintf(tracefile,"DS: %p\n",fp);
    fprintf(tracefile,"DA: %p\n",fp);
    fprintf(tracefile,"DU: %p\n",fp);
  }
  //fprintf(stderr,"Finished unlinking\n");
}

void flx_collector_t::post_delete(void *fp)
{
  Judy1Set(&j_tmp,(Word_t)fp,&je);
  if(tracefile)
    fprintf(tracefile,"IT: %p\n",fp);

}

void flx_collector_t::delete_frame(void *fp)
{
  allocator->deallocate(fp);
}

size_t flx_collector_t::reap ()
{
  size_t count = 0;
  Word_t next=(Word_t)NULL;
  int res = Judy1First(j_tmp,&next,&je);
  if(tracefile)
    fprintf(tracefile,"FT: %p\n",next);
  while(res) {
    delete_frame((void*)next);
    ++count;
    res = Judy1Next(j_tmp,&next,&je);
    if(tracefile)
      fprintf(tracefile,"NT: %p\n",next);
  }
  Judy1FreeArray(&j_tmp,&je);
  if(tracefile)
    fprintf(tracefile,"CT:\n");
  if(debug)
  {
    fprintf(stderr,"[gc] Reaped %zu objects\n",count);
    fprintf(stderr,"[gc] Still allocated %zu objects occupying %zu bytes\n", get_allocation_count(), get_allocation_amt());
  }
  return count;
}


//#include <valgrind/memcheck.h>

/* This is the top level mark routine
 * Its job is to mark all objects that are reachable
 * so a subsequent reaping phase can delete all
 * the objects that are NOT marked
 *
 * This mark bit is the low bit of the RTTI shape object pointer
 * stored in the j_shape Judy1Array.
 *
 * The meaning of this bit alternates between calls to the collector.
 * Initially all objects are considered garbage and the flag is toggled
 * to indicate the object is reachable.
 *
 * On the next pass the reachable value is reconsidered to mean
 * garbage and the flag toggled again. This saves a pass over
 * all objects marking them garbage before then tracing roots
 * to find which ones are not.
 */

void flx_collector_t::mark(pthread::memory_ranges_t *px)
{
  // The recursion limit is a stopper so recursions
  // won't blow the machine stack and also wipe out the cache
  // regularly. Our overall routine is iterative with limited
  // recursion. The recursions are faster but the iteration
  // can handle data type like lists of millions of elements
  // which would otherwise recurse millions of times.
  //
  int reclimit = 64;
  if(debug)
    fprintf(stderr,"[gc] Collector: Running mark\n");

  // sanity check
  assert (root_count == roots.size());

  // the j_tmp Judy1 array is just a set of pointers which
  // we have not yet examined. When we find pointers we stash
  // them in this set rather than examining them immediately.
  // Later we come back and examine them. This buffers the recursion
  // a bit. The set has to be empty initially.
  assert(j_tmp == 0);
  if (gcthreads < 2)
    mark_single(px,reclimit);
  else
    mark_multi(px,reclimit,gcthreads);
}

static void run_mark_thread(mark_thread_context_t *mtc)
{
  mtc->collector->mark_thread(mtc);
}

void flx_collector_t::mark_multi(pthread::memory_ranges_t *px,int reclimit, int nthreads)
{
//fprintf(stderr, "starting %d mark threads\n", nthreads);
  j_tmp_waiting = 0;
  mark_thread_context_t mtc {this,px, reclimit};
  ::std::vector< ::std::thread> mark_threads;
  for (int i=0; i<gcthreads; ++i)
    mark_threads.push_back (::std::thread (run_mark_thread, &mtc));
  for (int i=0; i<gcthreads; ++i)
    mark_threads[i].join();
//fprintf(stderr, "multithread mark finished\n");
}

// this method is run simultaneously by multiple threads
void flx_collector_t::mark_thread(mark_thread_context_t *mtc)
{
//fprintf(stderr, "multithread mark thread running\n");
  int reclimit = mtc->reclimit;
  pthread::memory_ranges_t *px  = mtc->px;
  // px is a set of memory ranges representing the stacks
  // of all pthreads including this one at the point the
  // collector got invoked. All the other threads than this
  // one must be stopped. The stack are found by recording the
  // base stack value when launching the thread, and using
  // the value when a thread stops to allow collection as the
  // high value. The stack contains all the machine registers
  // at this point too, since we used a long_jmp into a local
  // variable to put the registers on the stack.
  if(px)
  {
    // for all pthreads
    std::vector<pthread::memory_range_t>::iterator end = (*px).end();
    for(
      std::vector<pthread::memory_range_t>::iterator i = (*px).begin();
      i != end;
      ++i
    )
    {
      // get the stack extent for one pthread
      pthread::memory_range_t range = *i;
      if(debug)
      {
        size_t n = (char*)range.e - (char*)range.b;
        fprintf(stderr, "[gc] Conservate scan of memory %p->%p, %zu bytes\n",range.b, range.e, n);
      }
      //VALGRIND_MAKE_MEM_DEFINED(range.b, (char*)range.e-(char*)range.b);
      void *end = range.e;
      // for all machine words on the stack
      // this WILL FAIL if the stack isn't an exact multiple
      // of the size of a machine word
      for ( void *i = range.b; i != end; i = (void*)((void**)i+1))
      {
        //if(debug)
        // fprintf(stderr, "[gc] Check if *%p=%p is a pointer\n",i,*(void**)i);
        // conservative scan of every word on every stack
        scan_object(*(void**)i, reclimit);
      }
      if(debug)
        fprintf(stderr, "[gc] DONE: Conservate scan of memory %p->%p\n",range.b, range.e);
    }
  }

  // Now scan all the registered roots
  if(debug)
    fprintf(stderr, "[gc] Scanning roots\n");
  rootmap_t::iterator const end = roots.end();
  for(
    rootmap_t::iterator i = roots.begin();
    i != end;
    ++i
  )
  {
    if(debug)
      fprintf(stderr, "[gc] Scanning root %p\n", (*i).first);
    scan_object((*i).first, reclimit);
  }

  // Now, scan the temporary set in j_tmp  until it is empty
  // When we're processing an object with scan_object
  // if its an actual Felix object we mark it reachable
  // and then scan all the pointers in it: usually those pointers
  // are not scanned immediately by scan object but simply put
  // into the set j_tmp to schedule them for scanning.
  //
  // Note: Judy1First finds the first key greater than or equal to the given one,
  // it returns 0 if there is no such key.
  Word_t toscan;
  int res;
again:
  {
    ::std::unique_lock< ::std::mutex> dummy(j_tmp_lock);
retry:
    toscan = 0;
    res = Judy1First(j_tmp,&toscan,&je); // get one object scheduled for scanning
    if (!res) {
       ++j_tmp_waiting;
       if (j_tmp_waiting == gcthreads) {
         j_tmp_cv.notify_all();
         goto endoff;
       }
       j_tmp_cv.wait(dummy);
       --j_tmp_waiting;
       goto retry;
    }
    Judy1Unset(&j_tmp,toscan,&je);         // remove it immediately
  }
  scan_object((void*)toscan, reclimit);  // scan it, it will either be marked or discarded
  goto again;

endoff:
  assert(j_tmp == 0);

  if(debug)
    fprintf(stderr, "[gc] Done Scanning roots\n");
}



void flx_collector_t::mark_single(pthread::memory_ranges_t *px, int reclimit)
{
  // px is a set of memory ranges representing the stacks
  // of all pthreads including this one at the point the
  // collector got invoked. All the other threads than this
  // one must be stopped. The stack are found by recording the
  // base stack value when launching the thread, and using
  // the value when a thread stops to allow collection as the
  // high value. The stack contains all the machine registers
  // at this point too, since we used a long_jmp into a local
  // variable to put the registers on the stack.
  if(px)
  {
    // for all pthreads
    std::vector<pthread::memory_range_t>::iterator end = (*px).end();
    for(
      std::vector<pthread::memory_range_t>::iterator i = (*px).begin();
      i != end;
      ++i
    )
    {
      // get the stack extent for one pthread
      pthread::memory_range_t range = *i;
      if(debug)
      {
        size_t n = (char*)range.e - (char*)range.b;
        fprintf(stderr, "[gc] Conservate scan of memory %p->%p, %zu bytes\n",range.b, range.e, n);
      }
      //VALGRIND_MAKE_MEM_DEFINED(range.b, (char*)range.e-(char*)range.b);
      void *end = range.e;
      // for all machine words on the stack
      // this WILL FAIL if the stack isn't an exact multiple
      // of the size of a machine word
      for ( void *i = range.b; i != end; i = (void*)((void**)i+1))
      {
        //if(debug)
        // fprintf(stderr, "[gc] Check if *%p=%p is a pointer\n",i,*(void**)i);
        // conservative scan of every word on every stack
        scan_object(*(void**)i, reclimit);
      }
      if(debug)
        fprintf(stderr, "[gc] DONE: Conservate scan of memory %p->%p\n",range.b, range.e);
    }
  }

  // Now scan all the registered roots
  if(debug)
    fprintf(stderr, "[gc] Scanning roots\n");
  rootmap_t::iterator const end = roots.end();
  for(
    rootmap_t::iterator i = roots.begin();
    i != end;
    ++i
  )
  {
    if(debug)
      fprintf(stderr, "[gc] Scanning root %p\n", (*i).first);
    scan_object((*i).first, reclimit);
  }

  // Now, scan the temporary set in j_tmp  until it is empty
  // When we're processing an object with scan_object
  // if its an actual Felix object we mark it reachable
  // and then scan all the pointers in it: usually those pointers
  // are not scanned immediately by scan object but simply put
  // into the set j_tmp to schedule them for scanning.
  //
  // Note: Judy1First finds the first key greater than or equal to the given one,
  // it returns 0 if there is no such key.
  Word_t toscan = 0;
  int res = Judy1First(j_tmp,&toscan,&je); // get one object scheduled for scanning
  //if(tracefile)
  //  fprintf(tracefile,"FT: %p\n",toscan);
  while(res) {
    Judy1Unset(&j_tmp,toscan,&je);         // remove it immediately
    if(tracefile)
      fprintf(tracefile,"DT: %p\n",toscan);
    scan_object((void*)toscan, reclimit);  // scan it, it will either be marked or discarded
    toscan = 0;
    res = Judy1First(j_tmp,&toscan,&je);
    if(tracefile)
      fprintf(tracefile,"FT: %p\n",toscan);
  }
  assert(j_tmp == 0);

  if(debug)
    fprintf(stderr, "[gc] Done Scanning roots\n");
}



size_t flx_collector_t::sweep()
{
  if(debug)
    fprintf(stderr,"[gc] Collector: Sweep, garbage bit value=%d\n",(int)parity);
  size_t sweeped = 0;
  void *current = NULL;
  Word_t *pshape = (Word_t*)JudyLFirst(j_shape,(Word_t*)&current,&je); // GE
  if(tracefile)
    fprintf(tracefile,"FS: %p\n",current);
  if(pshape==(Word_t*)PPJERR)judyerror("sweep");

  while(pshape!=NULL)
  {
    if((*pshape & (uintptr_t)1) == (parity & (uintptr_t)1))
    {
      if(debug)
        fprintf(stderr,"[gc] Garbage   %p=%s[%zd][%zu/%zu] [#a=%zu,#d=%zu]\n",
          current,
          SHAPE(*pshape)->cname,
          SHAPE(*pshape)->count,
          get_used(current),
          get_count(current),
          SHAPE(*pshape)->allocations,
          SHAPE(*pshape)->deallocations
        );
      ++ sweeped;
      //fprintf(stderr,"Incr deallocation count ..\n");
      //++((gc_shape_t *)(*pshape & ~(uintptr_t)1))->deallocations;
      //fprintf(stderr,"Unlinking ..\n");
      unlink(current);
      //fprintf(stderr,"Posting delete ..\n");
      post_delete(current);
      //fprintf(stderr,"Reaping done\n");
    }
    else
    {
      if(debug)
        fprintf(stderr,"[gc] Reachable %p=%s[%zd][%zu/%zu] [#a=%zu,#d=%zu]\n",
          current,
          SHAPE(*pshape)->cname,
          SHAPE(*pshape)->count,
          get_used(current),
          get_count(current),
          SHAPE(*pshape)->allocations,
          SHAPE(*pshape)->deallocations
        );
    }

    //fprintf(stderr,"Calling Judy for next object\n");
    pshape = (Word_t*)JudyLNext(j_shape,(Word_t*)(void*)&current,&je); // GT
    if(tracefile)
      fprintf(tracefile,"NS: %p\n",current);
    //fprintf(stderr,"Judy got next object %p\n",pshape);
  }

  parity = !parity;
  if(debug)
    fprintf(stderr,"[gc] Sweeped %zu\n",sweeped);
  return reap();
}

void flx_collector_t::impl_add_root(void *memory)
{
  if(!memory)
  {
    fprintf(stderr, "[gc] GC ERROR: ADD NULL ROOT\n");
    abort();
  }
  rootmap_t::iterator iter = roots.find(memory);
  if(iter == roots.end())
  {
    std::pair<void *const, size_t> entry(memory,(uintptr_t)1);
    if(debug)
      fprintf(stderr,"[gc] Add root %p=%s\n", memory,get_shape(memory)->cname);
    roots.insert (entry);
    root_count++;
  }
  else {
    if(debug)
      fprintf(stderr,"[gc] Increment root %p to %zu\n", memory, (*iter).second+1);
    ++(*iter).second;
  }
}

void flx_collector_t::impl_remove_root(void *memory)
{
  rootmap_t::iterator iter = roots.find(memory);
  if(iter == roots.end())
  {
    fprintf(stderr, "[gc] GC ERROR: REMOVE ROOT WHICH IS NOT ROOT\n");
    abort();
  }
  if((*iter).second == (uintptr_t)1)
  {
    if(debug)
      fprintf(stderr,"[gc] Remove root %p\n", memory);
    roots.erase(iter);
    root_count--;
  }
  else {
    if(debug)
      fprintf(stderr,"[gc] Decrement root %p to %zu\n", memory, (*iter).second-1);
    --(*iter).second;
  }
}

/* This is the fun bit!
 * Register pointer is called by scan object, indirectly
 * via the custom scanner.
 * It then recursively calls scan_object on that pointer,
 * providing a standard recursive descent.
 *
 * HOWEVER if the recursion limit is reached during this process,
 * instead of recursing it just stashes the pointer in the
 * j_tmp collection for later processing.
 *
 * So recursions over small tree structures proceed as normal,
 * but when you get a long list or array to handle the recursion
 * is stopped before it blows the stack, and the data is just stashed
 * for later processing by the top level iterative loop
 */

// unfortunately requires a dynamic test to determine
// if we're using the threaded mark routine or not
void flx_collector_t::register_pointer(void *q, int reclimit)
{
  if (inrange(q)) {
    if(reclimit==0)
    {
      if(gcthreads>1)
      {
        ::std::unique_lock< ::std::mutex> dummy(j_tmp_lock);
        Judy1Set(&j_tmp,(Word_t)q,&je);
        j_tmp_cv.notify_one();
      }
      else {
        Judy1Set(&j_tmp,(Word_t)q,&je);
      }
      if(tracefile)
        fprintf(tracefile,"IT: %p\n",q);
    }
    else scan_object(q, reclimit-1);
  }
}

::flx::gc::generic::pointer_data_t flx_collector_t::get_pointer_data (void *p)
{
  ::flx::gc::generic::pointer_data_t pdat;
  pdat.head = NULL;
  pdat.max_elements = 0;
  pdat.used_elements = 0;
  pdat.shape = NULL;
  pdat.pointer = p;

  Word_t cand = (Word_t)p;
  Word_t head = cand;
  Word_t *ppshape = (Word_t*)JudyLLast(j_shape,&head, &je);
  if(tracefile)
    fprintf(tracefile,"LS: %p\n",head);
  if(ppshape==(Word_t*)PPJERR)judyerror("get_pointer_data");
  if(ppshape == NULL) return pdat; // no lower object
  gc_shape_t *pshape = SHAPE(*ppshape);
  size_t max_slots = get_count((void*)head);
  size_t used_slots = get_used((void*)head);
  size_t n = max_slots * pshape->count * pshape->amt;
  if(cand >= (Word_t)(void*)((unsigned char*)(void*)head+n)) return pdat; // not interior
  pdat.head = (void*)head;
  pdat.max_elements = max_slots;
  pdat.used_elements = used_slots;
  pdat.shape = pshape;
  return pdat;
}

/* Given some word siuze value p, we have to decide what it is.
 * If its a pointer into an allocated object, since we got here
 * that object is reachable so we ensure that object is marked
 * reachable so it won't be reaped
 */

// if a pointer is interior, then
// if marked reachable already return NULL,NULL
// else mark as reachable and return head,shape
flx_collector_t::memdata_t flx_collector_t::check_interior (void *p)
{
  Word_t reachable = (parity & (uintptr_t)1) ^ (uintptr_t)1;
  if(debug)
    fprintf(stderr,"[gc] Scan object %p, reachable bit value = %d\n",p,(int)reachable);

  // Now find the shape of the object into which the pointer points,
  // if it is a Felix allocated object. First, we use JudyLLast
  // which finds the value less than or equal to the given key.
  if (!inrange(p)) return memdata_t{NULL,NULL,0};
  Word_t cand = (Word_t)p;
  Word_t head=cand;
  Word_t *ppshape = (Word_t*)JudyLLast(j_shape,&head,&je);
  if(ppshape==(Word_t*)PPJERR)judyerror("check_interior");

  // if the pointer returned by Judy is NULL, there is no
  // allocated object at or lower then the given address so exit
  if(ppshape == NULL) return memdata_t{NULL,NULL,0}; // no lower object
  /*
  if(debug)
  {
    fprintf(stderr,"Found candidate object %p, &shape=%p, shape(1) %p\n",(void*)fp,(void*)w,(void*)(*w));
    fprintf(stderr," .. type=%s!\n",((gc_shape_t*)(*w & ~(uintptr_t)1))->cname);
  }
  */

  // if the object lower then the given pointer is already
  // marked reachable, there's nothing to do (all the pointers
  // it reaches should also be marked) so just exit.
  if( (*ppshape & (uintptr_t)1) == reachable) return memdata_t {NULL,NULL,0};   // already handled

  // get the actual shape of the candidate object
  // don't forget to mask out the low bit which is the reachability flag
  gc_shape_t *pshape = SHAPE(*ppshape);

  // calculate the length of the candidate object in bytes
  size_t exterior_count = get_count((void*)head);
  size_t n = exterior_count * pshape->count * pshape->amt;

  // if our pointer is greater than or equal to the "one past the end"
  // pointer of the object, it is not a pointer interior to that object
  // but a foreign pointer and must be ignored
  if(cand >= (Word_t)(void*)((unsigned char*)(void*)head+n)) return memdata_t{NULL,NULL,0}; // not interior
  if(debug)
    fprintf(stderr,"[gc] MARKING object %p, shape %p, type=%s\n",(void*)head,pshape,pshape->cname);

  // otherwise we have an iterior or head pointer to the object
  // so set the reachable flag in the judy shape array
  *ppshape = (*ppshape & ~(uintptr_t)1) | reachable;
  return memdata_t {(void*)head,pshape,n};
}

void flx_collector_t::scan_object(void *p, int reclimit)
{

  // CAN p be NULL?? If so a fast exit could be done
  // no point if it can't be null though

  // The reachability flag is the low bit object type pointer.
  // The sense of the flag alternative between 0 and 1 meaning
  // reachable on successive collections. This is an optimisation
  // which saves marking all object unreachable first, then marking
  // the reachable ones reachable. We just use the previous reachable
  // marking to mean unreachable next time, then flip the bit for each
  // reachable object. The value parity records the sense and is flipped
  // at the start of each GC pass.
  //Word_t reachable = (parity & (uintptr_t)1) ^ (uintptr_t)1;
again:
   memdata_t memdata = check_interior(p);
   if(memdata.head == NULL) return;
/*
  //if(debug)
  //  fprintf(stderr,"[gc] Scan object %p, reachable bit value = %d\n",p,(int)reachable);

  // Now find the shape of the object into which the pointer points,
  // if it is a Felix allocated object. First, we use JudyLLast
  // which finds the value less than or equal to the given key.
  if (!inrange(p)) return;
  Word_t cand = (Word_t)p;
  Word_t head=cand;
  Word_t *ppshape = (Word_t*)JudyLLast(j_shape,&head,&je);
  if(ppshape==(Word_t*)PPJERR)judyerror("scan_object");

  // if the pointer returned by Judy is NULL, there is no
  // allocated object at or lower then the given address so exit
  if(ppshape == NULL) return; // no lower object

  //if(debug)
  //{
  //  fprintf(stderr,"Found candidate object %p, &shape=%p, shape(1) %p\n",(void*)fp,(void*)w,(void*)(*w));
  //  fprintf(stderr," .. type=%s!\n",((gc_shape_t*)(*w & ~(uintptr_t)1))->cname);
  //}
  //

  // if the object lower then the given pointer is already
  // marked reachable, there's nothing to do (all the pointers
  // it reaches should also be marked) so just exit.
  if( (*ppshape & (uintptr_t)1) == reachable) return;   // already handled

  // get the actual shape of the candidate object
  // don't forget to mask out the low bit which is the reachability flag
  gc_shape_t *pshape = SHAPE(*ppshape);

  // calculate the length of the candidate object in bytes
  size_t n = get_count((void*)head) * pshape->count * pshape->amt;

  // if our pointer is greater than or equal to the "one past the end"
  // pointer of the object, it is not a pointer interior to that object
  // but a foreign pointer and must be ignored
  if(cand >= (Word_t)(void*)((unsigned char*)(void*)head+n)) return; // not interior
  if(debug)
    fprintf(stderr,"[gc] MARKING object %p, shape %p, type=%s\n",(void*)head,pshape,pshape->cname);

  // otherwise we have an iterior or head pointer to the object
  // so set the reachable flag in the judy shape array
  *ppshape = (*ppshape & ~(uintptr_t)1) | reachable;
*/

  // Now we have to look for pointers contained in the object

  // The first branch here is not used at the moment,
  // and is a hard coded way to do a conservative scan on the object

  if(memdata.pshape->flags & gc_flags_conservative)
  {
    size_t n_used = get_used((void*)memdata.head) * memdata.pshape->count;
    // end of object, rounded down to size of a void*
    void **end = (void**)(
      (unsigned char*)(void*)memdata.head +
      n_used * memdata.nbytes / sizeof(void*) * sizeof(void*)
    );
    for ( void **i = (void**)memdata.head; i != end; i = i+1)
    {
      if(debug)
      //  fprintf(stderr, "Check if *%p=%p is a pointer\n",i,*(void**)i);
      if(reclimit==0) {

// LOCK REQUIRED XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        Judy1Set(&j_tmp,(Word_t)*i,&je);
// END LOCK XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
        if(tracefile)
          fprintf(tracefile,"IT: %p\n",*i);
      }
      else
        scan_object(*i,reclimit -1);
    }
  }

  // This is the normal processing.
  else
  {
    // Calculate the dynamic count of used elements in the object.
    // All Felix objects are varrays which have an allocated and used
    // element count. The RTTI object always describes one element.
    size_t dyncount = get_used((void*)memdata.head);

    // if don't have a scanner for the object it is atomic,
    // that is it contains no pointers.
    // Otherwise call the scanner.
    if(memdata.pshape->scanner) {
      void *r = memdata.pshape->scanner(this, memdata.pshape,memdata.head,dyncount,reclimit);
      // If the scanner returns a non-zero value it is the sole pointer
      // in the object. So reset our argument and jump to the start of
      // this routine: self-tail-recursion optimisation.
      if (r) { p = r; goto again; }
      // Otherwise the scanner has registered the pointers it found that
      // need further examination. We do not do that examination here
      // recursively, or inside the scanner, because it might blow the stack.
      // Instead we just return, so a flat iteration loop can grab things
      // out of the registered pointer buffer and drive the process
      // with a flat loop.
    }
  }
}



size_t flx_collector_t::impl_collect()
{
  // THIS IS A BIT OF A HACK
  // but world_stop() is bugged!!
  // This is a temporary fix.
  FLX_SAVE_REGS;
  if (thread_control == NULL || thread_control->world_stop())
  {
    //if(debug)
    //  fprintf(stderr,"[gc] Collecting, thread %lx\n", (size_t)flx::pthread::get_current_native_thread());
    pthread::memory_ranges_t * mr = thread_control? thread_control -> get_block_list() : NULL;
    mark(mr);
    delete mr;
    size_t collected = sweep();
    if(thread_control) thread_control->world_start();
    //if(debug)
    //  fprintf(stderr,"[gc] FINISHED collect, thread %lx\n", (size_t)flx::pthread::get_current_native_thread());
    return collected;
  }
  else {
    if(debug)
      fprintf(stderr,"[gc] RACE: someone else is collecting, just yield\n");
    if(thread_control)thread_control->yield();
    return 0ul;
  }
}

void flx_collector_t::impl_free_all_mem()
{
  //fprintf(stderr,"impl_free_all_mem -- freeing roots\n");
  roots.clear();
  root_count = 0;
  //fprintf(stderr,"freeing all heap with sweep()\n");
  sweep();
}

flx_collector_t::~flx_collector_t()
{
   if(tracefile) {
     fclose(tracefile);
     fprintf(stderr,"Closed FLX_TRACE_GC file\n");
   }

  //THIS IS VERY DANGEROUS! What if don't want to collect
  //the garbage for efficiency reasons???
  //
  // ELIDED .. already caused a bug!
  //
  //free_all_mem();
}

}}} // end namespaces

