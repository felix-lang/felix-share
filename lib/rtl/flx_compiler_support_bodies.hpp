#line 134 "/Users/skaller/felix/src/packages/rtl.fdoc"
#ifndef __FLX_COMPILER_SUPPORT_BODIES_H__
#define __FLX_COMPILER_SUPPORT_BODIES_H__
#include "flx_compiler_support_headers.hpp"

#include <algorithm>

//
// convert an rvalue to an lvalue
template<typename T>
T const &lvalue(T const &x)
{
  return x;
}

// this reinterpret cast works with rvalues too
template<typename T, typename U>
T &reinterpret(U const &x) {
  return reinterpret_cast<T&>(const_cast<U&>(x));
}

// dflt init
template<typename T>
void dflt_init(T *p){ new(p) T(); }

// destroy object
template<typename T>
void destroy(T *p){ p->T::~T(); }

// copy initialise
template<typename T>
void copy_init (T *dst, T *src)
{
  new(dst) T(*src);
}

// move initialise
template<typename T>
void move_init (T *dst, T *src)
{
  new(dst) T(::std::move(*src));
}

// move initialise, destroy src
template<typename T>
void dmove_init (T *dst, T *src)
{
  new(dst) T(::std::move(*src));
  destroy (src);
}

// copy assign
template<typename T>
void copy_assign (T *dst, T *src)
{
  *dst = *src;
}

// move assign
template<typename T>
void move_assign (T *dst, T *src)
{
  *dst = ::std::move(*src);
}

// move assign, destroy src
template<typename T>
void dmove_assign (T *dst, T *src)
{
  *dst = ::std::move(*src);
  destroy (src);
}

class ValueType
{
  virtual size_t object_size_impl()=0;
  virtual size_t object_alignment_impl()=0;
  virtual void dflt_init_impl (void *)=0;
  virtual void destroy_impl (void *)=0;
  virtual void copy_init_impl(void *, void *)=0;
  virtual void move_init_impl(void *, void *)=0;
  virtual void copy_assign_impl(void *, void *)=0;
  virtual void move_assign_impl(void *, void *)=0;
public:
  size_t object_size() { return object_size_impl(); }
  size_t object_alignment() { return object_size_impl(); }
  void dflt_init(void *dst) { dflt_init_impl(dst); }
  void destroy(void *dst) { destroy_impl (dst); }

  void copy_init (void *dst, void *src) { copy_init_impl(dst,src); }
  void move_init (void *dst, void *src) { move_init_impl(dst,src); }
  void copy_assign(void *dst, void *src) { copy_assign_impl(dst,src); }
  void move_assign(void *dst, void *src) { move_assign_impl(dst,src); }
};

template<typename T>
class CxxValueType : public virtual ValueType
{
  size_t object_size_impl() { return sizeof(T); }
  size_t object_alignment_impl() { return alignof(T); }
  void dflt_init_impl(void *dst) { ::dflt_init<T>((T*)dst); }
  void destroy_impl(void *dst) { ::dflt_init<T>((T*)dst); }
  void copy_init_impl(void *dst, void *src) { ::copy_init<T>((T*)dst,(T*)src); }
  void move_init_impl(void *dst, void *src) { ::move_init<T>((T*)dst,(T*)src); }
  void copy_assign_impl(void *dst, void *src) { ::copy_assign<T>((T*)dst,(T*)src); }
  void move_assign_impl(void *dst, void *src) { ::move_assign<T>((T*)dst,(T*)src); }
};

// object does NOT own the product description array
// should use a shared pointer thing I guess
class ProductType : public virtual ValueType
{
  size_t n;
  ValueType **cp;
public:
  ProductType (ValueType **p, size_t m) : cp(p), n(n) {}
  ~ProductType();
  size_t object_size_impl() override;
  size_t object_alignment_impl() override;
  void dflt_init_impl (void *) override;
  void destroy_impl (void *) override;
  void copy_init_impl(void *, void *) override;
  void move_init_impl(void *, void *) override;
  void copy_assign_impl(void *, void *) override;
  void move_assign_impl(void *, void *) override;
};


template<typename T0, typename T1>
struct _tt2 {
  T0 mem_0;
  T1 mem_1;
  _tt2() {}
  _tt2 (T0 _a0, T1 _a1) : mem_0(_a0), mem_1(_a1) {}
};

template<typename T0, typename T1, typename T2>
struct _tt3 {
  T0 mem_0;
  T1 mem_1;
  T2 mem_2;
  _tt3() {}
  _tt3 (T0 _a0, T1 _a1, T2 _a2) :
    mem_0(_a0), mem_1(_a1),mem_2(_a2)
    {}
};

template<typename T0, typename T1, typename T2, typename T3>
struct _tt4 {
  T0 mem_0;
  T1 mem_1;
  T2 mem_2;
  T3 mem_3;
  _tt4() {}
  _tt4 (T0 _a0, T1 _a1, T2 _a2, T3 _a3) :
    mem_0(_a0), mem_1(_a1),mem_2(_a2), mem_3(_a3)
    {}
};

template<typename T0, typename T1, typename T2, typename T3, typename T4>
struct _tt5 {
  T0 mem_0;
  T1 mem_1;
  T2 mem_2;
  T3 mem_3;
  T4 mem_4;
  _tt5() {}
  _tt5 (T0 _a0, T1 _a1, T2 _a2, T3 _a3, T4 _a4) :
    mem_0(_a0), mem_1(_a1),mem_2(_a2), mem_3(_a3), mem_4(_a4)
    {}
};


#define FLX_EXEC_FAILURE(f,op,what) \
  throw ::flx::rtl::flx_exec_failure_t (f,op,what)

#define FLX_HALT(f,sl,sc,el,ec,s) \
  throw ::flx::rtl::flx_halt_t (::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),__FILE__,__LINE__,s)

// note call should be trace(&v,...) however that requires
// compiler support to make a trace record for each tracepoint
// so we use NULL for now

#ifdef FLX_ENABLE_TRACE
#define FLX_TRACE(v,f,sl,sc,el,ec,s) \
  ::flx::rtl::flx_trace (NULL,::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),__FILE__,__LINE__,s)
#else
#define FLX_TRACE(v,f,sl,sc,el,ec,s)
#endif

#define FLX_MATCH_FAILURE(f,sl,sc,el,ec) \
  throw ::flx::rtl::flx_match_failure_t (::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),__FILE__,__LINE__)

#define FLX_DROPTHRU_FAILURE(f,sl,sc,el,ec) \
  throw ::flx::rtl::flx_dropthru_failure_t (::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),__FILE__,__LINE__)

#define FLX_ASSERT_FAILURE(f,sl,sc,el,ec) \
  throw ::flx::rtl::flx_assert_failure_t (::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),__FILE__,__LINE__)

#define FLX_ASSERT2_FAILURE(f,sl,sc,el,ec,f2,sl2,sc2,el2,ec2) \
  throw ::flx::rtl::flx_assert2_failure_t (\
    ::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),\
    ::flx::rtl::flx_range_srcref_t(f2,sl2,sc2,el2,sc2),\
    __FILE__,__LINE__)

#define FLX_AXIOM_CHECK_FAILURE(f,sl,sc,el,ec,f2,sl2,sc2,el2,ec2) \
  throw ::flx::rtl::flx_axiom_check_failure_t (\
    ::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),\
    ::flx::rtl::flx_range_srcref_t(f2,sl2,sc2,el2,sc2),\
    __FILE__,__LINE__)

#define FLX_RANGE_FAILURE(mi,v,ma,f,sl,sc,el,ec) \
  throw ::flx::rtl::flx_range_failure_t (mi,v,ma,::flx::rtl::flx_range_srcref_t(f,sl,sc,el,ec),__FILE__,__LINE__)

// for generated code in body file
#define INIT_PC pc=0;
    ///< interior program counter

#if FLX_CGOTO
  #ifdef __clang__
  #define FLX_START_SWITCH (&&_start_switch); _start_switch: if(pc)goto *pc;
  #else
  #define FLX_START_SWITCH _start_switch: if(pc)goto *pc;
  #endif
  #define FLX_LOCAL_LABEL_ADDRESS(x) &&case_##x
  #define FLX_SET_PC(x) pc=&&case_##x;
  #define FLX_CASE_LABEL(x) case_##x:;
  #define FLX_DECLARE_LABEL(n,i,x) \
    extern void f##i##_##n##_##x(void) __asm__("l"#i"_"#n"_"#x);
  #define FLX_LABEL(n,i,x) x:\
    __asm__(".global l"#i"_"#n"_"#x);\
    __asm__("l"#i"_"#n"_"#x":");\
    __asm__(""::"g"(&&x));
  #define FLX_FARTARGET(n,i,x) (void*)&f##i##_##n##_##x
  #define FLX_END_SWITCH \
    _flx_dead_frame: throw ::flx::rtl::flx_dead_frame_failure_t(__FILE__,__LINE__);
#else
  #define FLX_START_SWITCH _start_switch: switch(pc){case 0:;
  #define FLX_LOCAL_LABEL_ADDRESS(x) x
  #define FLX_SET_PC(x) pc=x;
  #define FLX_CASE_LABEL(x) case x:;
  #define FLX_DECLARE_LABEL(n,i,x)
  #define FLX_LABEL(n,i,x) case n: x:;
  #define FLX_FARTARGET(n,i,x) n
  #define FLX_END_SWITCH \
    case -1: throw ::flx::rtl::flx_dead_frame_failure_t(__FILE__,__LINE__);\
    default: throw ::flx::rtl::flx_switch_failure_t(__FILE__,__LINE__); }
#endif

//
// We do a direct long jump to a target as follows:
//
// If the target frame is just ourself (this)
// we set the pc and just goto the start of the procedure,
// allowing the switch/computed goto there to do the local jump.
//
// If the target is foreign, we force the foreign frame pc
// to the target pc, and then return that frame to the driver
// so it will resume that procedure, executing the starting switch,
// which now jumps to the required location.
//
#define FLX_DIRECT_LONG_JUMP(ja) \
  { \
    ::flx::rtl::jump_address_t j = ja; \
    if(j.target_frame == this) { \
      pc = j.local_pc; \
      goto _start_switch; \
    } else { \
      j.target_frame->pc = j.local_pc; \
      return j.target_frame; \
    } \
  }

#define FLX_RETURN \
{ \
  con_t *tmp = _caller; \
  _caller = 0; \
  return tmp; \
}

#define FLX_NEWP(x) new(*PTF gcp,x##_ptr_map,true)x

#define FLX_FINALISER(x) \
static void x##_finaliser(::flx::gc::generic::collector_t *, void *__p){\
  ((x*)__p)->~x();\
}


#define FLX_FMEM_INIT_ONLY : ptf(_ptf)
#define FLX_FMEM_INIT : ptf(_ptf),
#define FLX_FPAR_PASS_ONLY ptf
#define FLX_FPAR_PASS ptf,
#define FLX_APAR_PASS_ONLY _ptf
#define FLX_APAR_PASS _ptf,
#define _PTF _ptf->
#define _PTFV _ptf
#define FLX_PASS_PTF 1
#define FLX_EAT_PTF(x) x
#define FLX_DEF_THREAD_FRAME

#define FLX_FRAME_WRAPPERS(mname,name) \
extern "C" FLX_EXPORT mname::thread_frame_t *name##_create_thread_frame(\
  ::flx::gc::generic::gc_profile_t *gcp\
) {\
  mname::thread_frame_t *p = new(*gcp,mname::thread_frame_t_ptr_map,false) mname::thread_frame_t();\
  p->gcp = gcp;\
  return p;\
}

// init is a heap procedure
#define FLX_START_WRAPPER(mname,name,x)\
extern "C" FLX_EXPORT ::flx::rtl::con_t *name##_flx_start(\
  mname::thread_frame_t *__ptf,\
  int argc,\
  char **argv,\
  FILE *stdin_,\
  FILE *stdout_,\
  FILE *stderr_\
) {\
  __ptf->argc = argc;\
  __ptf->argv = argv;\
  __ptf->flx_stdin = stdin_;\
  __ptf->flx_stdout = stdout_;\
  __ptf->flx_stderr = stderr_;\
  return (new(*__ptf->gcp,mname::x##_ptr_map,false) \
    mname::x(__ptf)) ->call(0);\
}

// init is a stack procedure
#define FLX_STACK_START_WRAPPER_PTF(mname,name,x)\
extern "C" FLX_EXPORT ::flx::rtl::con_t *name##_flx_start(\
  mname::thread_frame_t *__ptf,\
  int argc,\
  char **argv,\
  FILE *stdin_,\
  FILE *stdout_,\
  FILE *stderr_\
) {\
  __ptf->argc = argc;\
  __ptf->argv = argv;\
  __ptf->flx_stdin = stdin_;\
  __ptf->flx_stdout = stdout_;\
  __ptf->flx_stderr = stderr_;\
  mname::x(__ptf).stack_call();\
  return 0;\
}


// init is a stack procedure, no PTF
#define FLX_STACK_START_WRAPPER_NOPTF(mname,name,x)\
extern "C" FLX_EXPORT ::flx::rtl::con_t *name##_flx_start(\
  mname::thread_frame_t *__ptf,\
  int argc,\
  char **argv,\
  FILE *stdin_,\
  FILE *stdout_,\
  FILE *stderr_\
) {\
  __ptf->argc = argc;\
  __ptf->argv = argv;\
  __ptf->flx_stdin = stdin_;\
  __ptf->flx_stdout = stdout_;\
  __ptf->flx_stderr = stderr_;\
  mname::x().stack_call();\
  return 0;\
}


// init is a C procedure, passed PTF
#define FLX_C_START_WRAPPER_PTF(mname,name,x)\
extern "C" FLX_EXPORT ::flx::rtl::con_t *name##_flx_start(\
  mname::thread_frame_t *__ptf,\
  int argc,\
  char **argv,\
  FILE *stdin_,\
  FILE *stdout_,\
  FILE *stderr_\
) {\
  __ptf->argc = argc;\
  __ptf->argv = argv;\
  __ptf->flx_stdin = stdin_;\
  __ptf->flx_stdout = stdout_;\
  __ptf->flx_stderr = stderr_;\
  mname::x(__ptf);\
  return 0;\
}

// init is a C procedure, NOT passed PTF
#define FLX_C_START_WRAPPER_NOPTF(mname,name,x)\
extern "C" FLX_EXPORT ::flx::rtl::con_t *name##_flx_start(\
  mname::thread_frame_t *__ptf,\
  int argc,\
  char **argv,\
  FILE *stdin_,\
  FILE *stdout_,\
  FILE *stderr_\
) {\
  mname::x();\
  return 0;\
}


#endif
