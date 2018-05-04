#line 78 "/Users/skaller/felix/src/packages/dynlink.fdoc"
#ifndef __FLX_DL_H__
#define __FLX_DL_H__

// define dynamic library loader stuff, even for static linkage
// SPECS:
//
// FLX_LIBHANDLE is the type of a native DLL handle
//   it's a typedef NOT a macro.
//
// FLX_NOLIBRARY is a macro specifying the value if there is no library.
//   used for initialisation, or, error value if dynamic load fails
//
// FLX_LIB_EXTENSION is a macro specifying the string name of
//  the platform library extension including the dot (.)
//
// FLX_ENV_LIBRARY_PATH_NAME is a macro that specifies the name
//   of the environment variable specifying extra directories
//   to search for DLLs.
//
// FLX_NATIVE_DLSYM(lib,sym) accepts a library handle and an identifier.
//   It works for both static and dynamic linkage.
//
//   For dynamic linkage it converts the symbol to a string
//     and calls dlsym
//   For static linkage it just returns the provided symbol,
//     which should have been linked by the linker.
//     This will work for both static linkage AND for
//     load time dynamic linkage (but not run time linkage).
//
// FLX_NATIVE_SDLSYM(lib,string) accepts a library handle
//   and a string.  If lib is a valid run time loaded library,
//   this routine works independently of how it was linked
//   (since even statically linked programs can dlopen libraries).
//
//   It may even work for lib=NULL if the linker is set to export
//   symbols to the program, and NULL is the linkers code for the
//   module's namespace.
//
// FLX_DLSYM(lib,sym) is just FLX_NATIVE_DLSYM, it requires a symbol.
//
// FLX_SDLSYM(lib,string) uses FLX_NATIVE_SDLSYM if dynamic linkage is selected
//   and throws an exception if static linkage is chosen.
//
// Therefore:
//   * the "S" version of these macros uses a string name,
//     the non-"S" version uses an identifier.
//
//   * FLX_NATIVE_SDLSYM uses a string name and always does
//     run time lookup.
//
//   * FLX_DLSYM uses a symbol and uses a linker bound
//     address if FLX_STATIC_LINK is selected
//     Otherwise it uses run time lookup.
//
#if FLX_WIN32
  #include <windows.h>
  typedef HMODULE FLX_LIBHANDLE;
  #define FLX_LIB_EXTENSION ".DLL"
  #define FLX_NATIVE_DLSYM(x,y) (void*)GetProcAddress(x,#y)
  #define FLX_NATIVE_SDLSYM(x,y) (void*)GetProcAddress(x,y)
  #define FLX_ENV_LIBRARY_PATH_NAME "PATH"
#else
  // UNIX, recent OSX
  typedef void *FLX_LIBHANDLE;
  #if FLX_CYGWIN
    #define FLX_LIB_EXTENSION ".dll"
    #define FLX_ENV_LIBRARY_PATH_NAME "LD_LIBRARY_PATH"
  #elif FLX_MACOSX
    #define FLX_LIB_EXTENSION ".dylib"
    #define FLX_ENV_LIBRARY_PATH_NAME "DYLD_LIBRARY_PATH"
  #else
    #define FLX_LIB_EXTENSION ".so"
    #define FLX_ENV_LIBRARY_PATH_NAME "LD_LIBRARY_PATH"
  #endif
  #include <dlfcn.h>
  #define FLX_NATIVE_DLSYM(x,y) dlsym(x,#y)
  #define FLX_NATIVE_SDLSYM(x,y) dlsym(x,y)
#endif

#define FLX_NOLIBRARY NULL

#define FLX_DLSYM(x,y) FLX_NATIVE_DLSYM(x,y)

#ifndef FLX_STATIC_LINK
  #define FLX_SDLSYM(x,y) FLX_NATIVE_SDLSYM(x,(y))
#else
  #define FLX_SDLSYM(x,y) (throw ::flx::rtl::flx_link_failure_t(\
    "<static link>",y,"dlsym with static link requires name not string"),\
    (void*)0\
  )
#endif
#endif

