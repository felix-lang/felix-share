#line 1655 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"

#ifndef __FLX_TS_COLLECTOR_H__
#define __FLX_TS_COLLECTOR_H__
#include "flx_collector.hpp"
#include "pthread_thread.hpp"
#include <thread>
#include <mutex>

namespace flx {
namespace gc {
namespace collector {

/// Naive thread safe Mark and Sweep Collector.
struct PTHREAD_EXTERN flx_ts_collector_t :
  public flx::gc::collector::flx_collector_t
{
  flx_ts_collector_t(allocator_t *, flx::pthread::thread_control_t *, int _gcthreads, FILE*);
  ~flx_ts_collector_t();

private:
  /// allocator
  void *v_allocate(gc_shape_t *ptr_map, size_t);

  /// collector (returns number of objects collected)
  size_t v_collect();

  // add and remove roots
  void v_add_root(void *memory);
  void v_remove_root(void *memory);

  // statistics
  size_t v_get_allocation_count()const;
  size_t v_get_root_count()const;
  size_t v_get_allocation_amt()const;

private:
  mutable ::std::mutex mut;
};


}}} // end namespaces

#endif
