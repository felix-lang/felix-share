#line 44 "/Users/skaller/felix/src/packages/exceptions.fdoc"
#include "flx_continuation.hpp"
namespace flx { namespace rtl {
// ********************************************************
// con_t implementation
// ********************************************************

con_t::con_t() : pc(0), p_svc(0), _caller(0) {
#if FLX_DEBUG_CONT
 fprintf(stderr,"Constructing %p\n",this);
#endif
}
con_t::~con_t(){
#if FLX_DEBUG_CONT
  fprintf(stderr,"Destroying %p\n",this);
#endif
}
}} // namespaces

