#line 303 "/Users/skaller/felix/src/packages/dynlink.fdoc"
#ifndef __FLX_DYNLINK_H__
#define __FLX_DYNLINK_H__
#include "flx_rtl.hpp"
#include "flx_gc.hpp"
#include "flx_dl.h"
#include "flx_dlopen.hpp"
#include "flx_exceptions.hpp"
#include "flx_continuation.hpp"

#include <string>

namespace flx { namespace dynlink {

struct DYNLINK_EXTERN flx_dynlink_t;
struct DYNLINK_EXTERN flx_libinst_t;


/// frame creators.
typedef void *(*thread_frame_creator_t)
(
  ::flx::gc::generic::gc_profile_t*
);

/// library initialisation routine.
typedef ::flx::rtl::con_t *(*start_t)
(
  void*,
  int,
  char **,
  FILE*,
  FILE*,
  FILE*

);

typedef ::flx::rtl::con_t *(*main_t)(void*);

/// dynamic object loader.
struct DYNLINK_EXTERN flx_dynlink_t
{
  // filename of library used for dynamic linkage
  ::std::string filename;

  // modulename of library
  // usually filename without path prefix or extension
  ::std::string modulename;

  // OS specific handle refering to the library if one is loaded
  // undefine otherwise
  FLX_LIBHANDLE library;

  // Felix specific entry point used to create thread frame.
  // Typically this function allocates the thread frame as a C++
  // object, calling its contructor.
  // A library together with a thread frame is known as an instance
  // of the library.
  thread_frame_creator_t thread_frame_creator;

  // Felix specific entry point used to initialise thread frame
  // Morally equivalent to the body of a C++ constructor,
  // this calls the libraries initialisation routine.
  // If the library is meant to be a program, this routine
  // often contains the program code.
  start_t start_sym;

  // A separate mainline, morally equivalent to C main() function.
  // Intended to be called after the start routine has completed.
  main_t main_sym;

  // Allow a default initialised default object refering to no library.
  flx_dynlink_t(bool debug);

  // set static link data into an empty dynlink object.
  void static_link(
    ::std::string modulename,
    thread_frame_creator_t thread_frame_creator,
    start_t start_sym,
    main_t main_sym);


  // initialise for static link
  // equivalent to default object followed by call to static_link method
  flx_dynlink_t(
    ::std::string modulename,
    thread_frame_creator_t thread_frame_creator,
    start_t start_sym,
    main_t main_sym,
    bool debug
  ) throw(::flx::rtl::flx_link_failure_t);

  // dynamic link library from filename and module name
  void dynamic_link_with_modulename(
     const ::std::string& filename,
     const ::std::string& modulename) throw(::flx::rtl::flx_link_failure_t);

  // With this variant the module name is calculated from the filename.
  void dynamic_link(const ::std::string& filename) throw(::flx::rtl::flx_link_failure_t);

  virtual ~flx_dynlink_t();

  bool debug;


private:
  void unlink(); // implementation of destructor only
  flx_dynlink_t(flx_dynlink_t const&); // uncopyable
  void operator=(flx_dynlink_t const&); // uncopyable
};

/// Thread Frame Initialisation.

struct DYNLINK_EXTERN flx_libinst_t
{
  void *thread_frame;
  ::flx::rtl::con_t *start_proc;
  ::flx::rtl::con_t *main_proc;
  flx_dynlink_t *lib;
  ::flx::gc::generic::gc_profile_t *gcp;
  bool debug;

  void create
  (
    flx_dynlink_t *lib_a,
    ::flx::gc::generic::gc_profile_t *gcp_a,
    int argc,
    char **argv,
    FILE *stdin_,
    FILE *stdout_,
    FILE *stderr_,
    bool debug_
  );

  void destroy ();

  ::flx::rtl::con_t *bind_proc(void *fn, void *data);
  virtual ~flx_libinst_t();
  flx_libinst_t(bool debug);

private:
  flx_libinst_t(flx_libinst_t const&);
  void operator=(flx_libinst_t const&);
};

DYNLINK_EXTERN extern ::flx::gc::generic::gc_shape_t flx_dynlink_ptr_map;
DYNLINK_EXTERN extern ::flx::gc::generic::gc_shape_t flx_libinst_ptr_map;

}} // namespaces
#endif

