#line 1075 "/Users/skaller/felix/src/packages/rtl.fdoc"
#include "flx_executil.hpp"
namespace flx { namespace rtl { namespace executil {
void run(::flx::rtl::con_t *p)
{
  while(p)
  {
    try { p=p->resume(); }
    catch (::flx::rtl::con_t *x) { p = x; }
  }
}

void frun (::flx::gc::generic::gc_profile_t* gcp, ::flx::rtl::con_t *p)
{
  ::std::list< ::flx::rtl::fthread_t*> *q =
    new ::std::list<::flx::rtl::fthread_t*>()
  ;

  ::flx::run::sync_sched *ss =
     new ::flx::run::sync_sched(false, gcp, q)
  ;

  ::flx::rtl::fthread_t *ft =
    new(*gcp,::flx::rtl::_fthread_ptr_map,false) ::flx::rtl::fthread_t(p)
  ;

  ss->collector->add_root(ft);
  ss->active->push_back(ft);
  ss->frun();
  if (ss->ft) ss->collector->remove_root(ss->ft);
  for(
    ::std::list<::flx::rtl::fthread_t*>::iterator pf = ss->active->begin();
    pf != ss->active->end();
    pf++
  )
  ss->collector->remove_root(*pf);
  delete ss->active; delete ss->ft; delete ss;
}

}}}
