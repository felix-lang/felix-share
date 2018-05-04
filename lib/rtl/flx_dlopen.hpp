#line 179 "/Users/skaller/felix/src/packages/dynlink.fdoc"
#ifndef __FLX_DLOPEN_H__
#define __FLX_DLOPEN_H__
#include "flx_dynlink_config.hpp"
#include "flx_dl.h"

#include <string>
using namespace std;

namespace flx { namespace dynlink {
/// Load library
DYNLINK_EXTERN FLX_LIBHANDLE flx_load_library_nothrow(const ::std::string& filename);
DYNLINK_EXTERN FLX_LIBHANDLE flx_load_library_throw(const ::std::string& filename);

DYNLINK_EXTERN FLX_LIBHANDLE flx_load_module_nothrow(const ::std::string& filename);
DYNLINK_EXTERN FLX_LIBHANDLE flx_load_module_throw(const ::std::string& filename);

DYNLINK_EXTERN ::std::string flx_lib_extension ();
DYNLINK_EXTERN ::std::string flx_env_library_path_name ();

DYNLINK_EXTERN FLX_LIBHANDLE flx_nolibrary();

DYNLINK_EXTERN void *flx_native_dlsym
  (FLX_LIBHANDLE,::std::string);

}}

#endif
