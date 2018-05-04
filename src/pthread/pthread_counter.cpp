#line 807 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#include "pthread_counter.hpp"
#include <stdio.h>

namespace flx { namespace pthread {


flx_ts_counter_t::flx_ts_counter_t() : x(0) {}
flx_ts_counter_t::flx_ts_counter_t(long init) : x(init) {}

flx_ts_counter_t::~flx_ts_counter_t() {
  wait_zero();
}

long flx_ts_counter_t::pre_incr() {
  ::std::unique_lock< ::std::mutex> l(m);
  ++x;
  return x;
}

long flx_ts_counter_t::pre_decr() {
  ::std::unique_lock< ::std::mutex> l(m);
  --x;
  if(x==0) c.notify_all();
  return x;
}

long flx_ts_counter_t::post_incr() {
  ::std::unique_lock< ::std::mutex> l(m);
  ++x;
  return x+1;
}

long flx_ts_counter_t::post_decr() {
  ::std::unique_lock< ::std::mutex> l(m);
  --x;
  if(x==0) c.notify_all();
  return x+1;
}

long flx_ts_counter_t::decr_pos() {
  ::std::unique_lock< ::std::mutex> l(m);
  if(x>0)--x;
  if(x==0) c.notify_all();
  return x;
}

long flx_ts_counter_t::get() {
  ::std::unique_lock< ::std::mutex> l(m);
  return x;
}

long flx_ts_counter_t::set(long a) {
  ::std::unique_lock< ::std::mutex> l(m);
  x = a;
  return x;
}

long flx_ts_counter_t::swap(long a) {
  ::std::unique_lock< ::std::mutex> l(m);
  long tmp = x;
  x = a;
  if(x==0) c.notify_all();
  return tmp;
}

void flx_ts_counter_t::wait_zero() {
  ::std::unique_lock< ::std::mutex> l(m);
  while(1){
    if(x==0)return;
    c.wait(m);
  }
}

void flx_ts_counter_t::decr_wait_zero() {
  ::std::unique_lock< ::std::mutex> l(m);
   --x;
   while (x>0) c.wait(m);
}

}}

