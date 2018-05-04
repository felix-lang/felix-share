#line 454 "/Users/skaller/felix/src/packages/dynlink.fdoc"
#include "flx_dynlink.hpp"
#include "flx_strutil.hpp"
#include <stdio.h>
#include <cstring>
#include <cstdlib>
#include <stddef.h>

namespace flx { namespace dynlink {

flx_dynlink_t::flx_dynlink_t(flx_dynlink_t const&) {} // no copy hack
void flx_dynlink_t::operator=(flx_dynlink_t const&) {} // no copy hack

flx_dynlink_t::flx_dynlink_t(bool debug_):
  filename(""),
  modulename(""),
  library(0),
  thread_frame_creator(NULL),
  start_sym(NULL),
  main_sym(NULL),
  debug(debug_)
{}

flx_dynlink_t::flx_dynlink_t(
  ::std::string modulename_a,
  thread_frame_creator_t thread_frame_creator,
  start_t start_sym,
  main_t main_sym,
  bool debug_
  ) throw(::flx::rtl::flx_link_failure_t)
:
  modulename (modulename_a),
  library(0),
  thread_frame_creator(thread_frame_creator),
  start_sym(start_sym),
  main_sym(main_sym),
  debug(debug_)
{
  if(!thread_frame_creator)
    throw ::flx::rtl::flx_link_failure_t("<static link>","dlsym","create_thread_frame");

  if(!start_sym)
    throw ::flx::rtl::flx_link_failure_t("<static link>","dlsym","flx_start");
}

void flx_dynlink_t::static_link (
  ::std::string modulename,
  thread_frame_creator_t thread_frame_creator,
  start_t start_sym,
  main_t main_sym
)
{
  this->modulename = modulename;
  this->thread_frame_creator = thread_frame_creator;
  this->start_sym = start_sym;
  this->main_sym = main_sym;
}


void flx_dynlink_t::dynamic_link_with_modulename(const ::std::string& filename_a, const ::std::string& modulename_a) throw(::flx::rtl::flx_link_failure_t)
{
  filename = filename_a;
  modulename = modulename_a;
  library = flx_load_library_throw(filename);
  //fprintf(stderr,"File %s dlopened at %p ok\n",fname.c_str(),library);

  thread_frame_creator = (thread_frame_creator_t)
    FLX_NATIVE_SDLSYM(library,(modulename+"_create_thread_frame").c_str());
  if(!thread_frame_creator)
    throw ::flx::rtl::flx_link_failure_t(filename,"dlsym",modulename+"_create_thread_frame");

  if (debug)
    fprintf(stderr,"[dynlink:dynamic_link] Thread frame creator found at %p\n",thread_frame_creator);

  start_sym = (start_t)FLX_NATIVE_SDLSYM(library,(modulename+"_flx_start").c_str());
  if (debug)
    fprintf(stderr,"[dynlink:dynamic_link] Start symbol = %p\n",start_sym);
  if(!start_sym)
    throw ::flx::rtl::flx_link_failure_t(filename,"dlsym",modulename+"_flx_start");

  main_sym = (main_t)FLX_NATIVE_SDLSYM(library,"flx_main");

  if(debug)
    fprintf(stderr,"[dynlink:dynamic_link] main symbol = %p\n",main_sym);

}

void flx_dynlink_t::dynamic_link(const ::std::string& filename_a) throw(::flx::rtl::flx_link_failure_t)
{
  string mname = ::flx::rtl::strutil::filename_to_modulename (filename_a);
  dynamic_link_with_modulename(filename_a,mname);
}

// dont actually unload libraries
// it doesn't work right in C/C++
// can leave dangling references
// impossible to manage properly
void flx_dynlink_t::unlink()
{
    //fprintf(stderr,"closing library\n");
//#if FLX_WIN32 || FLX_CYGWIN
#if FLX_WIN32
    //FreeLibrary(library);
#else
    //dlclose(library);
#endif
}

flx_dynlink_t::~flx_dynlink_t() {
  // fprintf(stderr, "Library %p of module '%s' file '%s' destroyed\n", this,
  // modulename.c_str(), filename.c_str()
  // );
}
#line 570 "/Users/skaller/felix/src/packages/dynlink.fdoc"

// ************************************************
// libinst
// ************************************************

flx_libinst_t::~flx_libinst_t() {
  // fprintf(stderr, "Library instance %p of library %p destroyed\n",this,lib);
}
flx_libinst_t::flx_libinst_t(bool debug_) :
  thread_frame (NULL),
  start_proc (NULL),
  main_proc (NULL),
  lib (NULL),
  gcp(NULL),
  debug(debug_)
{}

flx_libinst_t::flx_libinst_t(flx_libinst_t const&){}
void flx_libinst_t::operator=(flx_libinst_t const&){}

void flx_libinst_t::create
(
  flx_dynlink_t *lib_a,
  flx::gc::generic::gc_profile_t *gcp_a,
  int argc,
  char **argv,
  FILE *stdin_,
  FILE *stdout_,
  FILE *stderr_,
  bool debug_
)
{
  lib = lib_a;
  gcp = gcp_a;
  debug = debug_;
  if (debug)
    fprintf(stderr,"[libinst:create] Creating instance for library %p->'%s'\n",lib, lib->filename.c_str());
  if (debug)
    fprintf(stderr, "[libinst:create] Creating thread frame\n");
  thread_frame = lib->thread_frame_creator( gcp);
  if (debug)
    fprintf(stderr, "[libinst:create] thread frame CREATED %p\n", thread_frame);
  if (debug)
    fprintf(stderr, "[libinst:create] CREATING start_proc by running start_sym %p\n", lib->start_sym);
  try {
    start_proc = lib->start_sym(thread_frame, argc, argv, stdin_,stdout_,stderr_);
  }
  catch (::flx::rtl::con_t *p) {
    if (debug)
    fprintf(stderr,
       "[lininst::create] setting start_proc to continuation %p thrown by start_sym %p\n",
       p,lib->start_sym);
    start_proc = p;
  }

  if (debug)
    fprintf(stderr, "[libinst:create] start_proc CREATED %p\n", start_proc);
  if (debug)
    fprintf(stderr, "[libinst:create] CREATING main_proc by running main_sym %p\n", lib->main_sym);
  main_proc = lib->main_sym?lib->main_sym(thread_frame):0;
  if (debug)
    fprintf(stderr, "[libinst:create] main_proc CREATED %p\n", main_proc);
}

::flx::rtl::con_t *flx_libinst_t::bind_proc(void *fn, void *data) {
  typedef ::flx::rtl::con_t *(*binder_t)(void *,void*);
  return ((binder_t)fn)(thread_frame,data);
}

// ********************************************************
// OFFSETS for flx_dynlink_t
// ********************************************************
FLX_FINALISER(flx_dynlink_t)
::flx::gc::generic::gc_shape_t flx_dynlink_ptr_map = {
  NULL,
  "dynlink::flx_dynlink_t",
  1,sizeof(flx_dynlink_t),
  flx_dynlink_t_finaliser,
  0, // fcops
  0, // private data
  0, // scanner
  ::flx::gc::generic::tblit<flx_dynlink_t>, // encoder
  ::flx::gc::generic::tunblit<flx_dynlink_t>,  // decoder
  ::flx::gc::generic::gc_flags_default, // flags
  0UL, 0UL
};


// ********************************************************
// OFFSETS for flx_libinst
// ********************************************************
static const std::size_t flx_libinst_offsets[4]={
    offsetof(flx_libinst_t,thread_frame),
    offsetof(flx_libinst_t,start_proc),
    offsetof(flx_libinst_t,main_proc),
    offsetof(flx_libinst_t,lib)
};
FLX_FINALISER(flx_libinst_t)
static ::flx::gc::generic::offset_data_t const flx_libinst_offset_data = { 4, flx_libinst_offsets };
::flx::gc::generic::gc_shape_t flx_libinst_ptr_map = {
  &flx_dynlink_ptr_map,
  "dynlink::flx_libinst",
  1,sizeof(flx_libinst_t),
  flx_libinst_t_finaliser,
  0, // fcops
  &flx_libinst_offset_data,
  ::flx::gc::generic::scan_by_offsets,
  ::flx::gc::generic::tblit<flx_libinst_t>,::flx::gc::generic::tunblit<flx_libinst_t>,
  ::flx::gc::generic::gc_flags_default,
  0UL, 0UL
};

}} // namespaces
