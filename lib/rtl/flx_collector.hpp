#line 810 "/Users/skaller/felix/src/packages/gc.fdoc"

#ifndef __FLX_COLLECTOR_H__
#define __FLX_COLLECTOR_H__
#include <cstddef>
#include "flx_gc.hpp"
#include <map>
#include "pthread_thread.hpp"
#include <Judy.h>

namespace flx {
namespace gc {
namespace collector {
using namespace generic;

struct GC_EXTERN malloc_free;
struct GC_EXTERN tracing_allocator;
struct GC_EXTERN flx_collector_t;

/// Allocator using malloc and free.
struct GC_EXTERN malloc_free : public virtual allocator_t
{
  void *allocate(::std::size_t);
  void deallocate(void *);
  ~malloc_free();
};

/// Allocator which saves allocations and deallocations
/// to a file, delegating operations to a servant allocator
struct GC_EXTERN tracing_allocator : public virtual allocator_t
{
  allocator_t *servant;
  FILE *tracefile;
  tracing_allocator(FILE *, allocator_t *);
  void *allocate(::std::size_t);
  void deallocate(void *);
  ~tracing_allocator();
};


struct mark_thread_context_t
{
  flx_collector_t *collector;
  pthread::memory_ranges_t *px;
  int reclimit;
};


/// Naive Mark and Sweep Collector.
struct GC_EXTERN flx_collector_t : public collector_t
{
  flx_collector_t(allocator_t *, flx::pthread::thread_control_base_t *, int _gcthreads, FILE *tf);
  ~flx_collector_t();

  // RF: added to allow implementation of non-leaky drivers.
  void impl_free_all_mem(); // clear all roots, sweep.

  void set_used(void *memory, size_t);
  void incr_used(void *memory, ptrdiff_t);
  size_t get_used(void *memory);
  size_t get_count(void *memory);
  void *create_empty_array( gc_shape_t *shape, size_t count);
  gc_shape_t *get_shape(void *memory);
  flx::pthread::thread_control_base_t *get_thread_control()const;
  void register_pointer(void *q, int reclimit);
  ::flx::gc::generic::pointer_data_t get_pointer_data(void *);

protected:

  /// allocator
  void *impl_allocate(gc_shape_t *ptr_map, size_t);

  /// collector (returns number of objects collected)
  size_t impl_collect();

  // add and remove roots
  void impl_add_root(void *memory);
  void impl_remove_root(void *memory);

  //
  void check();

  // statistics
  size_t impl_get_allocation_count()const;
  size_t impl_get_root_count()const;
  size_t impl_get_allocation_amt()const;
  void impl_finalise(void *fp);

private:
  /// allocator
  void *v_allocate(gc_shape_t *ptr_map, size_t);

  /// collector (returns number of objects collected)
  size_t v_collect();

  // add and remove roots
  void v_add_root(void *memory);
  void v_remove_root(void *memory);
  void v_free_all_mem();

  // statistics
  size_t v_get_allocation_count()const;
  size_t v_get_root_count()const;
  size_t v_get_allocation_amt()const;

private:
  void judyerror(char const*);
  size_t allocation_count;
  size_t root_count;
  size_t allocation_amt;

  uintptr_t minptr;
  uintptr_t maxptr;

  bool inrange(void *p)const { return minptr <= uintptr_t(p) && uintptr_t(p) < maxptr; }
  void unlink(void *frame);
  void v_finalise(void *frame);
  void post_delete(void *frame);
  void delete_frame(void *frame);
  size_t reap();

  // top level mark, calls mark_single or mark_multi
  void mark(pthread::memory_ranges_t*);

  // single threaded mark
  void mark_single(pthread::memory_ranges_t*, int);

  // multithreaded mark: single thread enters and creates
  // worker threads which run mark_thread routine below
  void mark_multi(pthread::memory_ranges_t*,int reclimit, int nthreads);

public: // unfortunately, due to dispatch machinery
  // worker thread
  void mark_thread(mark_thread_context_t *);

private:
  int gcthreads;
  size_t sweep(); // calls scan_object

  typedef std::map<void *,size_t, std::less<void *> > rootmap_t;
  rootmap_t roots;
  bool parity;
  allocator_t *allocator;
  flx::pthread::thread_control_base_t *thread_control;


  // JudyL array and error object
  void *j_shape;
  void *j_nalloc;
  void *j_nused;
  FILE *tracefile;
public:
  struct memdata_t {
    void *head;
    gc_shape_t *pshape;
    size_t nbytes;
  };
  void scan_object(void *memory, int reclimit);
  memdata_t check_interior (void *memory);

  ::std::mutex j_tmp_lock;
  ::std::condition_variable j_tmp_cv;
  int j_tmp_waiting;
  void *j_tmp;
  JError_t je;
};

}}} // end namespaces
#endif

