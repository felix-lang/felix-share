#line 211 "/Users/skaller/felix/src/packages/dynlink.fdoc"
#include "flx_dlopen.hpp"
#include "flx_exceptions.hpp"
#include <cstdlib>
#include <stdio.h>

namespace flx { namespace dynlink {

FLX_LIBHANDLE
flx_load_library_nothrow(const std::string& filename)
{
  FLX_LIBHANDLE library = FLX_NOLIBRARY;
  if (::std::getenv("FLX_SHELL_ECHO")!=(char*)0)
    fprintf(stderr,"[load_library] %s\n", filename.c_str());
#if FLX_WIN32
  // stop windows showing err dialogues, ignoring error code.
  (void)SetErrorMode(SEM_NOOPENFILEERRORBOX);
  library = LoadLibrary(filename.c_str());
#else
    library = dlopen(filename.c_str(),RTLD_NOW | RTLD_LOCAL);
#endif
  return library;
}

FLX_LIBHANDLE
flx_load_library_throw(const ::std::string& filename)
{
  FLX_LIBHANDLE library = flx_load_library_nothrow(filename);
  if(library == FLX_NOLIBRARY)
    throw ::flx::rtl::flx_link_failure_t(filename,"LoadLibrary/dlopen","Cannot find dll/shared library");
  return library;
}

FLX_LIBHANDLE
flx_load_module_nothrow(const ::std::string& filename)
{
  return flx_load_library_nothrow(filename + FLX_LIB_EXTENSION);
}

FLX_LIBHANDLE
flx_load_module_throw(const ::std::string& filename)
{
  return flx_load_library_throw(filename + FLX_LIB_EXTENSION);
}

::std::string flx_lib_extension () { return FLX_LIB_EXTENSION; }
::std::string flx_env_library_path_name () { return FLX_ENV_LIBRARY_PATH_NAME; }

FLX_LIBHANDLE flx_nolibrary() { return FLX_NOLIBRARY; }

void *flx_native_dlsym(FLX_LIBHANDLE lib, ::std::string symname)
{
  return FLX_NATIVE_DLSYM(lib,symname.c_str());
}

}} // namespaces
