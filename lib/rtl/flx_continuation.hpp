#line 19 "/Users/skaller/felix/src/packages/exceptions.fdoc"
#ifndef _FLX_CONTINUATION_HPP
#define _FLX_CONTINUATION_HPP
#include "flx_exceptions_config.hpp"
#include "flx_compiler_support_headers.hpp"

// ********************************************************
/// CONTINUATION.
// ********************************************************

namespace flx {namespace rtl {
struct FLX_EXCEPTIONS_EXTERN con_t ///< abstract base for mutable continuations
{
  FLX_PC_DECL               ///< interior program counter
  struct _uctor_ *p_svc;           ///< pointer to service request

  con_t();                  ///< initialise pc, p_svc to 0
  virtual con_t *resume()=0;///< method to perform a computational step
  virtual ~con_t();
  con_t * _caller;          ///< callers continuation (return address)
};
}} //namespaces
#endif
