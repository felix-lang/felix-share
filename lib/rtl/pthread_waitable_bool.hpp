#line 891 "/Users/skaller/felix/src/packages/rtl-threads.fdoc"
#ifndef __FLX_PTHREAD_WAIT_BOOL_H__
#define __FLX_PTHREAD_WAIT_BOOL_H__
#include "flx_pthread_config.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>

namespace flx { namespace pthread {

// a waitable boolean.
class PTHREAD_EXTERN waitable_bool {
  ::std::mutex cv_lock;       // to work with the condition var
  ::std::condition_variable_any finished_cond;
  bool finished;   // might seem redundant, but that's how CVs work.
public:
  waitable_bool();

  void wait_until_true();
  void signal_true();
};

}} // namespace pthread, flx
#endif // __FLX_PTHREAD_WAIT_BOOL_H__

