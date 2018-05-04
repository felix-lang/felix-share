#line 1701 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "flx_rtl_config.hpp"
#include "flx_ts_collector.hpp"

namespace flx {
namespace gc {
namespace collector {

flx_ts_collector_t::flx_ts_collector_t(allocator_t *a, flx::pthread::thread_control_t *tc,int _gcthreads, FILE *tf) :
  flx_collector_t(a,tc,_gcthreads,tf)
{}

flx_ts_collector_t::~flx_ts_collector_t(){}

void *flx_ts_collector_t::v_allocate(gc_shape_t *ptr_map, size_t x) {
  ::std::unique_lock< ::std::mutex> dummy(mut);
  return impl_allocate(ptr_map,x);
}

size_t flx_ts_collector_t::v_collect() {
  // NO MUTEX
  //if(debug)
  //  fprintf(stderr,"[gc] Request to collect, thread_control = %p, thread %p\n", thread_control, (size_t)flx::pthread::get_current_native_thread());
  return impl_collect();
}

void flx_ts_collector_t::v_add_root(void *memory) {
  ::std::unique_lock< ::std::mutex> dummy(mut);
  impl_add_root(memory);
}

void flx_ts_collector_t::v_remove_root(void *memory) {
  ::std::unique_lock< ::std::mutex> dummy(mut);
  impl_remove_root(memory);
}

size_t flx_ts_collector_t::v_get_allocation_count()const {
  ::std::unique_lock< ::std::mutex> dummy(mut);
  return impl_get_allocation_count();
}

size_t flx_ts_collector_t::v_get_root_count()const {
  ::std::unique_lock< ::std::mutex> dummy(mut);
  return impl_get_root_count();
}

size_t flx_ts_collector_t::v_get_allocation_amt()const {
  ::std::unique_lock< ::std::mutex> dummy(mut);
  return impl_get_allocation_amt();
}


}}} // end namespaces


