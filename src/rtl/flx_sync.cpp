#line 111 "/Users/skaller/felix/src/packages/sync.fdoc"

#include <stdio.h>

#include "flx_sync.hpp"

using namespace flx::rtl;

namespace flx { namespace run {

char const *sync_sched::get_fstate_desc(fstate_t fs)
{
  switch(fs)
  {
    case blocked: return "blocked";
    case delegated: return "delegated";
    default: return "Illegal fstate_t";
  }
}

char const *sync_sched::get_fpc_desc()
{
  if (ft)
    return "Next request pos";
  else
  {
    if (active->size() > 0) return "Next fthread pos";
    if (waiting && waiting->size() > 0) return "Pop Waiting fthreads pos";
    else return "Out of active and waiting threads";
  }
}


sync_sched::sync_sched (
  bool debug_driver_,
  ::flx::gc::generic::gc_profile_t *gcp_,
  ::std::list<fthread_t*> *active_
) :
  debug_driver(debug_driver_),
  collector(gcp_->collector),
  active(active_),
  waiting(0),
  ft(0)
{}

// if the active list is not empty,
// take the top of the active list and make it current,
// popping it off the active list.
// If the active list is empty, make the current NULL.

void sync_sched::pop_current()
  {
     if(active->size() > 0)
     {
       ft = active->front();
       active->pop_front();
     }
     else if(waiting && waiting->size() > 0)
     {
       ft = waiting->front();
       waiting->pop_front();
       if(waiting->size() == 0) {
         delete waiting;
         waiting=0;
       }
     }
     else
       ft = 0;
  }

  void sync_sched::show_state () {
    if (debug_driver)
      fprintf(stderr, "CUR[%p] ACT[%p]\n",ft,
        active->size()?active->front():NULL);
  }

// if the current fibre is not NULL, forget it,
// then set the current fibre to the top of the
// active list and pop it
void sync_sched::forget_current()
  {
    if(ft)
    {
       collector->remove_root(ft);
       pop_current();
    }
  }


// make the argument f the current fibre
// if there was a non-NULL current fibre before,
// push it onto the active list
void sync_sched::push_old(fthread_t *f)
  {
    if(ft) active->push_front(ft);
    ft = f;
  }

// same as push_old except the argument is fresh
// so it is made a root first
void sync_sched::push_new(fthread_t *f)
  {
    collector->add_root(f);
    push_old(f);
  }

void sync_sched::do_yield()
    {
      if(debug_driver)
         fprintf(stderr,"[sync: svc_yield] yield");
      active->push_back(ft);
      pop_current();
    }

void sync_sched::do_swait()
    {
      if(debug_driver)
         fprintf(stderr,"[sync: svc_swait] swait\n");
      if(active->size() > 0) {
        if (waiting==0) waiting = new ::std::list<fthread_t*>;
        waiting->push_back(ft);
        pop_current();
      }
    }


void sync_sched::do_spawn_detached()
    {
      fthread_t *ftx = *(fthread_t**)request->data;
      if(debug_driver)
        fprintf(stderr,"[sync: svc_spawn_detached] Spawn fthread %p\n",ftx);
      push_new(ftx);
    }

void sync_sched::do_schedule_detached()
    {
      fthread_t *ftx = *(fthread_t**)request->data;
      if(debug_driver)
        fprintf(stderr,"[sync: svc_schedule_detached] Schedule fthread %p\n",ftx);
      collector->add_root(ftx);
      active->push_back(ftx);
    }

void sync_sched::do_sread()
    {
      readreq_t * pr = (readreq_t*)request->data;
      schannel_t *chan = pr->chan;
      if(debug_driver)
        fprintf(stderr,"[sync: svc_read] Fibre %p Request to read on channel %p\n",ft,chan);
      if(chan==NULL) goto svc_read_none;
    svc_read_next:
      {
        fthread_t *writer= chan->pop_writer();
        if(writer == 0) goto svc_read_none;       // no writers
        if(writer->cc == 0) goto svc_read_next;   // killed
        readreq_t * pw = (readreq_t*)writer->get_svc()->data;
        if(debug_driver)
          fprintf(stderr,"[sync: svc_read] Writer @%p=%p, read into %p\n",
            pw->variable,*(void**)pw->variable, pr->variable);
        if (pr->variable && pw->variable)
          *(void**)pr->variable = *(void**)pw->variable;
        if(debug_driver)
          fprintf(stderr,"[sync: svc_read] current fibre %p FED, fibre %p UNBLOCKED\n",ft, writer);

        // WE are the reader, stay current, push writer
        // onto active list
        active->push_front(writer);
        collector->add_root(writer);
show_state();
        return;
      }

    svc_read_none:
      if(debug_driver)
        fprintf(stderr,"[sync: svc_read] No writers on channel %p: fibre %p HUNGRY\n",chan,ft);
      chan->push_reader(ft);
      forget_current();
show_state();
      return;
    }

void sync_sched::do_swrite()
    {
      readreq_t * pw = (readreq_t*)request->data;
      schannel_t *chan = pw->chan;
      if(debug_driver)
         fprintf(stderr,"[sync: svc_write] Fibre %p Request to write on channel %p\n",ft,chan);
      if(chan==NULL)goto svc_write_none;
    svc_write_next:
      {
        fthread_t *reader= chan->pop_reader();
        if(reader == 0) goto svc_write_none;     // no readers
        if(reader->cc == 0) goto svc_write_next; // killed
        readreq_t * pr = (readreq_t*)reader->get_svc()->data;
        if(debug_driver)
          fprintf(stderr,"[sync: svc_write] Writer @%p=%p, read into %p\n",
            pw->variable,*(void**)pw->variable, pr->variable);
        if (pr->variable && pw->variable)
          *(void**)pr->variable = *(void**)pw->variable;
        if(debug_driver)
          fprintf(stderr,"[sync: svc_write] hungry fibre %p FED\n",reader);

        // WE are the writer, push us onto the active list
        // and make the reader on the channel current
        push_new (reader);
show_state();
        return;
      }
    svc_write_none:
      if(debug_driver)
        fprintf(stderr,"[sync: svc_write] No readers on channel %p: fibre %p BLOCKING\n",chan,ft);
      chan->push_writer(ft);
      forget_current();
show_state();
      return;
    }

void sync_sched::external_multi_swrite (schannel_t *chan, void *data)
    {
      if(chan==NULL) return;
    svc_multi_write_next:
      fthread_t *reader= chan->pop_reader();
      if(reader == 0)  return;    // no readers left
      if(reader->cc == 0) goto svc_multi_write_next; // killed
      {
        readreq_t * pr = (readreq_t*)reader->get_svc()->data;
        if(debug_driver)
           fprintf(stderr,"[sync: svc_multi_write] Write data %p, read into %p\n",
             data, pr->variable);
        if (pr->variable)
          *(void**)pr->variable = data;
        push_new(reader);
      }
      goto svc_multi_write_next;
    }

void sync_sched::do_multi_swrite()
    {
      readreq_t * pw = (readreq_t*)request->data;
      void *data = *(void**)pw->variable;
      schannel_t *chan = pw->chan;
      if(debug_driver)
        fprintf(stderr,"[sync: svc_multi_write] Request to write on channel %p\n",chan);
      external_multi_swrite (chan, data);
    }

void sync_sched::do_kill()
    {
      fthread_t *ftx = *(fthread_t**)request->data;
      if(debug_driver)fprintf(stderr,"[sync: svc_kill] Request to kill fthread %p\n",ftx);
      ftx -> kill();
      return;
    }


sync_sched::fstate_t sync_sched::frun()
{
  if (debug_driver)
     fprintf(stderr,"[sync] frun: entry ft=%p, active size=%zu\n", ft,active->size());
dispatch:
  if (ft == 0) pop_current();
  if (ft == 0) return blocked;
  request = ft->run();        // run fthread to get request
  if(request == 0)            // euthenasia request
  {
    if(debug_driver)
      fprintf(stderr,"[sync] unrooting fthread %p\n",ft);
    collector->remove_root(ft);
    ft = 0;
    goto dispatch;
  }

  if (debug_driver)
    fprintf(stderr,"[flx_sync:sync_sched] dispatching service request %d\n", request->variant);
  switch(request->variant)
  {
    case svc_yield: do_yield(); goto dispatch;

    case svc_swait: do_swait(); goto dispatch;

    case svc_spawn_detached: do_spawn_detached(); goto dispatch;

    case svc_sread: do_sread(); goto dispatch;

    case svc_swrite: do_swrite(); goto dispatch;

    case svc_multi_swrite: do_multi_swrite(); goto dispatch;

    case svc_kill: do_kill(); goto dispatch;

    default:
      return delegated;
  }
}

}}
