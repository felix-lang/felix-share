#line 1061 "/Users/skaller/felix/src/packages/rtl.fdoc"
#ifndef FLX_EXECUTIL
#define FLX_EXECUTIL
#include "flx_rtl_config.hpp"
#include "flx_rtl.hpp"
#include "flx_sync.hpp"
#include "flx_gc.hpp"

namespace flx { namespace rtl { namespace executil {
  RTL_EXTERN void run(flx::rtl::con_t *c);
  RTL_EXTERN void frun (::flx::gc::generic::gc_profile_t* gcp, ::flx::rtl::con_t *p);
}}}
#endif
