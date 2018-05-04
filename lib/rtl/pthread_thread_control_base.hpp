#line 117 "/Users/skaller/felix/src/packages/gc.fdoc"
#ifndef __PTHREAD_THREAD_CONTROL_BASE_HPP__
#define __PTHREAD_THREAD_CONTROL_BASE_HPP__

#include "flx_pthread_config.hpp"
#include <string.h>
#include <vector>

namespace flx { namespace pthread {

struct thread_data_t {
  thread_data_t(void *b) : stack_base(b), stack_top(0), active(true) {}
  void *stack_base;
  void *stack_top;
  bool active;
};

struct memory_range_t {
  memory_range_t(void *b_, void *e_) : b(b_), e(e_) {}
  void *b;
  void *e;
};

typedef ::std::vector<memory_range_t> memory_ranges_t;

class PTHREAD_EXTERN world_stop_notifier_t
{
public:
  virtual void notify_world_stop()=0;
  virtual ~world_stop_notifier_t();
};

class PTHREAD_EXTERN thread_control_base_t
{
public:
  virtual bool get_debug() const =0;
  virtual bool world_stop() = 0;
  virtual void world_start() = 0;
  virtual void resume() = 0;
  virtual void suspend() = 0;
  virtual void yield() = 0;
  virtual void join_all() = 0;
  virtual void add_thread(void*)=0;
  virtual void remove_thread()=0;
  virtual size_t thread_count()=0;
  virtual void register_world_stop_notifier(world_stop_notifier_t *)=0;
  virtual void unregister_world_stop_notifier(world_stop_notifier_t *)=0;

  virtual ~thread_control_base_t()=0;
  virtual  memory_ranges_t *get_block_list() = 0; // caller owns result and should delete it
};
}}
#endif
