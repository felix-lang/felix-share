#line 784 "/Users/skaller/felix/src/packages/rtl.fdoc"
#include "flx_rtl.hpp"
#include "flx_rtl_shapes.hpp"

#include <cstdio>
#include <cassert>
#include <cstddef>
#include <stdint.h>
#include "flx_exceptions.hpp"
#include "flx_collector.hpp"
#include "flx_serialisers.hpp"
#include "flx_continuation.hpp"

// main run time library code

namespace flx { namespace rtl {


static char const *svc_desc[13] = {
  "svc_yield",
  "svc_get_fthread",
  "svc_read",
  "svc_general",
  "svc_reserved1",
  "svc_spawn_pthread",
  "svc_spawn_detached",
  "svc_sread",
  "svc_swrite",
  "svc_kill",
  "svc_swait",
  "svc_multi_swrite",
  "svc_schedule_detached"
};

char const *describe_service_call(int x)
{
  if (x < 0 || x >12) return "Unknown service call";
  else return svc_desc[x];
}

// ********************************************************
// slist implementation
// ********************************************************

slist_t::slist_t(::flx::gc::generic::gc_profile_t *_gcp) : gcp (_gcp), head(0) {}

bool slist_t::isempty()const { return head == 0; }

void slist_t::push(void *data)
{
  head = new(*gcp,slist_node_ptr_map,true) slist_node_t(head,data);
}

// note: never fails, return NULL pointer if the list is empty
void *slist_t::pop()
{
  if(head) {
    void *data = head->data;
    head=head->next;
    return data;
  }
  else return 0;
}
// ********************************************************
// fthread_t implementation
// ********************************************************

fthread_t::fthread_t() : cc(0) {}
fthread_t::fthread_t(con_t *a) : cc(a) {}

// uncopyable object but implementation needed for linker????
//fthread_t::fthread_t(fthread_t const&){ assert(false); }
//void fthread_t::operator=(fthread_t const&){ assert(false); }

void fthread_t::kill() { cc = 0; }

_uctor_ *fthread_t::get_svc()const { return cc?cc->p_svc:0; }

_uctor_ *fthread_t::run() {
  if(!cc) return 0; // dead
restep:
  cc->p_svc = 0;
step:
  //fprintf(stderr,"[fthread_t::run::step] cc=%p->",cc);
  try { cc = cc->resume(); }
  catch (con_t *x) { cc = x; }

  //fprintf(stderr,"[fthread_t::run::step] ->%p\n",cc);
  if(!cc) return 0; // died

  if(cc->p_svc)
  {
    //fprintf(stderr,"[fthread_t::run::service call] ->%d\n",cc->p_svc);
    switch(cc->p_svc->variant)
    {
      case svc_get_fthread:
        // NEW VARIANT LAYOUT RULES
        // One less level of indirection here
        //**(fthread_t***)(cc->p_svc->data) = this;
        *(fthread_t**)(cc->p_svc->data) = this;
        goto restep;      // handled

      //case svc_yield:
      //  goto restep;

      // we don't know what to do with the request,
      // so pass the buck to the driver
      default:
        return cc->p_svc;
    }
  }
  goto step;
}

// ********************************************************
// schannel_t implementation
// ********************************************************

schannel_t::schannel_t (gc::generic::gc_profile_t *gcp) :
  waiting_to_read(0), waiting_to_write(0)
{
  waiting_to_read = new (*gcp, slist_ptr_map,false) slist_t(gcp);
  waiting_to_write = new (*gcp, slist_ptr_map,false) slist_t(gcp);
}

// uncopyable object but implementation needed for linker
//schannel_t::schannel_t(schannel_t const&) { assert(false); }
//void schannel_t::operator=(schannel_t const&) { assert(false); }

void schannel_t::push_reader(fthread_t *r)
{
  waiting_to_read->push(r);
}

void schannel_t::push_writer(fthread_t *w)
{
  waiting_to_write->push(w);
}

fthread_t *schannel_t::pop_reader()
{
  return (fthread_t*)waiting_to_read->pop();
}

fthread_t *schannel_t::pop_writer()
{
  return (fthread_t*)waiting_to_write->pop();
}
// ********************************************************
// trace feature
// ********************************************************

int flx_enable_trace=1;
size_t flx_global_trace_count=0uL;

void flx_trace(flx_trace_t* tr,flx_range_srcref_t sr, char const *file, int line, char const *msg)
{
  if(!flx_enable_trace)return;
  flx_global_trace_count++;
  if(tr)
  {
    tr->count++;
    if(tr->enable_trace)
    {
      fprintf(stderr,"%zu : %s\n",tr->count,msg);
      print_loc(stderr,sr,file,line);
    }
  }
  else
  {
    fprintf(stderr,"%zu : %s\n",flx_global_trace_count,msg);
    print_loc(stderr,sr,file,line);
  }
}
}}

ProductType::~ProductType(){}

size_t ProductType::object_size_impl() {
  size_t s = 0;
  for (int i=0; i<n; ++i) s+=cp[i]->object_size();
  return s;
}

size_t ProductType::object_alignment_impl() {
  size_t s = 0;
  for (int i=0; i<n; ++i) s = ::std::max(s,cp[i]->object_alignment());
  return s;
}

// if a is aligned then a%amt == 0
// otherwise a%amt is the amount over the previously aligned
// address, so we subtract it to get the previously aligned address
// and then add the amt back to get the next one.
uintptr_t round_up (uintptr_t a, size_t amt) {
  size_t adj = a % amt;
  return adj? a + amt - a%amt:a;
}
#define INCR(p,a) *(unsigned char **)p += a;

void *round_up (void *a, size_t amt) {
  return (void*)round_up((uintptr_t)a, amt);
}

void ProductType::dflt_init_impl (void *p) {
  for (int i = 0; i<n; ++i) {
    auto vt = cp[i];
    p = round_up(p,vt->object_alignment());
    vt->dflt_init(p);
    INCR(p,vt->object_size());
  }
};

void ProductType::destroy_impl (void *p) {
  for (int i = 0; i<n; ++i) {
    auto vt = cp[i];
    p = round_up(p,vt->object_alignment());
    vt->destroy(p);
    INCR(p,vt->object_size());
  }
}

void ProductType::copy_init_impl(void *dst, void *src) {
  for (int i = 0; i<n; ++i) {
    auto vt = cp[i];
    auto align = vt->object_alignment();
    src = round_up(src,align);
    dst = round_up(dst,align);
    vt->copy_init(dst,src);
    auto z = vt->object_size();
    INCR(src,z);
    INCR(dst,z);
  }
}

void ProductType::move_init_impl(void *dst, void *src) {
  for (int i = 0; i<n; ++i) {
    auto vt = cp[i];
    auto align = vt->object_alignment();
    src = round_up(src,align);
    dst = round_up(dst,align);
    vt->move_init(dst,src);
    auto z = vt->object_size();
    INCR(src, z);
    INCR(dst, z);
  }
}

void ProductType::copy_assign_impl(void *dst, void *src) {
  for (int i = 0; i<n; ++i) {
    auto vt = cp[i];
    auto align = vt->object_alignment();
    src = round_up(src,align);
    dst = round_up(dst,align);
    vt->copy_assign(dst,src);
    auto z = vt->object_size();
    INCR(src, z);
    INCR(dst, z);
  }
}

void ProductType::move_assign_impl(void *dst, void *src) {
  for (int i = 0; i<n; ++i) {
    auto vt = cp[i];
    auto align = vt->object_alignment();
    src = round_up(src,align);
    dst = round_up(dst,align);
    vt->move_assign(dst,src);
    auto z = vt->object_size();
    INCR(src, z);
    INCR(dst, z);
  }
}


